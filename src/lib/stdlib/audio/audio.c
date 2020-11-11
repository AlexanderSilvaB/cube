#include <cube/cubeext.h>
#include <math.h>
#include <soundio/soundio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct waveform_st
{
    int id;
    float *samples;
    int index, count;
    struct SoundIoOutStream *outstream;
    struct waveform_st *next;
    bool done;
} waveform_t;

typedef struct record_st
{
    int id;
    struct SoundIoRingBuffer *ring_buffer;
    struct SoundIoInStream *instream;
    struct record_st *next;
    bool done;
} record_t;

static int prioritized_sample_rates[] = {
    48000, 44100, 96000, 24000, 0,
};

static int record_id = 0;
static int waveform_id = 0;
struct SoundIo *soundio = NULL;
struct SoundIoDevice *out_device = NULL;
struct SoundIoDevice *in_device = NULL;
struct waveform_st *default_waveform = NULL;
struct record_st *default_record = NULL;
static int default_out_sample_rate = 0;
static int default_in_sample_rate = 0;

static int min_int(int a, int b)
{
    return (a < b) ? a : b;
}

static bool is_int(const char *str)
{
    int i = 0;
    while (str[i] != '\0')
    {
        if (str[i] < '0' || str[i] > '9')
            return false;
        i++;
    }
    return true;
}

static record_t *get_record(int id)
{
    record_t *record = default_record;
    while (record)
    {
        if (record->id == id)
            return record;

        record = record->next;
    }
    return NULL;
}

static waveform_t *get_waveform(int id)
{
    waveform_t *waveform = default_waveform;
    while (waveform)
    {
        if (waveform->id == id)
            return waveform;

        waveform = waveform->next;
    }
    return NULL;
}

static void read_callback(struct SoundIoInStream *instream, int frame_count_min, int frame_count_max)
{
    struct record_st *record = instream->userdata;
    struct SoundIoChannelArea *areas;
    int err;

    if (!record)
        return;

    char *write_ptr = soundio_ring_buffer_write_ptr(record->ring_buffer);
    int free_bytes = soundio_ring_buffer_free_count(record->ring_buffer);
    int free_count = free_bytes / instream->bytes_per_frame;

    if (free_count < frame_count_min || free_count == 0)
    {
        record->done = true;
        return;
    }

    int write_frames = min_int(free_count, frame_count_max);
    int frames_left = write_frames;

    for (;;)
    {
        int frame_count = frames_left;

        if ((err = soundio_instream_begin_read(instream, &areas, &frame_count)))
        {
            fprintf(stderr, "begin read error: %s", soundio_strerror(err));
            return;
        }

        if (!frame_count)
            break;

        if (!areas)
        {
            // Due to an overflow there is a hole. Fill the ring buffer with
            // silence for the size of the hole.
            memset(write_ptr, 0, frame_count * instream->bytes_per_frame);
        }
        else
        {
            for (int frame = 0; frame < frame_count; frame += 1)
            {
                for (int ch = 0; ch < instream->layout.channel_count; ch += 1)
                {
                    memcpy(write_ptr, areas[ch].ptr, instream->bytes_per_sample);
                    areas[ch].ptr += areas[ch].step;
                    write_ptr += instream->bytes_per_sample;
                }
            }
        }

        if ((err = soundio_instream_end_read(instream)))
        {
            fprintf(stderr, "end read error: %s", soundio_strerror(err));
            return;
        }

        frames_left -= frame_count;
        if (frames_left <= 0)
            break;
    }

    int advance_bytes = write_frames * instream->bytes_per_frame;
    soundio_ring_buffer_advance_write_ptr(record->ring_buffer, advance_bytes);
}

static void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max)
{
    const struct SoundIoChannelLayout *layout = &outstream->layout;
    struct SoundIoChannelArea *areas;
    waveform_t *waveform = (waveform_t *)outstream->userdata;
    if (!waveform)
        return;

    if (waveform->done)
        return;

    int frames_left = waveform->count - waveform->index;
    if (frames_left > frame_count_max)
        frames_left = frame_count_max;

    if (!frames_left)
    {
        waveform->done = true;
        return;
    }

    int err;
    while (frames_left > 0)
    {
        int frame_count = frames_left;

        if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count)))
        {
            fprintf(stderr, "%s\n", soundio_strerror(err));
            return;
        }

        if (!frame_count)
            break;

        for (int frame = 0; frame < frame_count; frame++)
        {
            float sample = waveform->samples[frame + waveform->index];
            for (int channel = 0; channel < layout->channel_count; channel += 1)
            {
                float *ptr = (float *)(areas[channel].ptr + areas[channel].step * frame);
                *ptr = sample;
            }
        }

        if ((err = soundio_outstream_end_write(outstream)))
        {
            fprintf(stderr, "%s\n", soundio_strerror(err));
            return;
        }

        waveform->index += frame_count;
        frames_left -= frame_count;
    }

    int end = 0;
}

static cube_native_var *get_layout_var(const struct SoundIoChannelLayout *layout)
{
    cube_native_var *list = NATIVE_LIST();
    if (layout->name)
    {
        ADD_NATIVE_LIST(list, NATIVE_STRING_COPY(layout->name));
    }
    else
    {
        for (int i = 0; i < layout->channel_count; i++)
        {
            ADD_NATIVE_LIST(list, NATIVE_STRING_COPY(soundio_get_channel_name(layout->channels[i])));
        }
    }
    return list;
}

static bool set_output_device(int device_index)
{
    if (device_index >= 0)
    {
        // Clear
        if (out_device)
            soundio_device_unref(out_device);
        out_device = NULL;

        // Set
        out_device = soundio_get_output_device(soundio, device_index);
        if (out_device)
        {
            default_out_sample_rate = 0;
            int *sample_rate_ptr;
            for (sample_rate_ptr = prioritized_sample_rates; *sample_rate_ptr; sample_rate_ptr += 1)
            {
                if (soundio_device_supports_sample_rate(out_device, *sample_rate_ptr))
                {
                    default_out_sample_rate = *sample_rate_ptr;
                    break;
                }
            }
            if (!default_out_sample_rate)
                default_out_sample_rate = out_device->sample_rates[0].max;
        }

        return out_device != NULL;
    }

    return false;
}

static bool set_input_device(int device_index)
{
    if (device_index >= 0)
    {
        // Clear
        if (in_device)
            soundio_device_unref(in_device);
        in_device = NULL;

        // Set
        in_device = soundio_get_input_device(soundio, device_index);
        if (in_device->probe_error)
        {
            fprintf(stderr, "Unable to probe device: %s\n", soundio_strerror(in_device->probe_error));
            soundio_device_unref(in_device);
            in_device = NULL;
        }
        else
        {
            soundio_device_sort_channel_layouts(in_device);
            default_in_sample_rate = 0;
            int *sample_rate_ptr;
            for (sample_rate_ptr = prioritized_sample_rates; *sample_rate_ptr; sample_rate_ptr += 1)
            {
                if (soundio_device_supports_sample_rate(in_device, *sample_rate_ptr))
                {
                    default_in_sample_rate = *sample_rate_ptr;
                    break;
                }
            }
            if (!default_in_sample_rate)
                default_in_sample_rate = in_device->sample_rates[0].max;
        }

        return in_device != NULL;
    }
    return false;
}

static int get_input_by_name(const char *name)
{
    int count = soundio_input_device_count(soundio);
    for (int i = 0; i < count; i++)
    {
        struct SoundIoDevice *device = soundio_get_input_device(soundio, i);
        if (strcmp(name, device->name) == 0)
            return i;
    }
    return -1;
}

static int get_output_by_name(const char *name)
{
    int count = soundio_output_device_count(soundio);
    for (int i = 0; i < count; i++)
    {
        struct SoundIoDevice *device = soundio_get_output_device(soundio, i);
        if (strcmp(name, device->name) == 0)
            return i;
    }
    return -1;
}

EXPORTED void cube_init()
{
    int err;
    soundio = soundio_create();
    if (!soundio)
        return;

    if ((err = soundio_connect(soundio)))
        return;

    soundio_flush_events(soundio);

    int default_out_device_index = soundio_default_output_device_index(soundio);
    set_output_device(default_out_device_index);

    int default_in_device_index = soundio_default_input_device_index(soundio);
    set_input_device(default_in_device_index);
}

EXPORTED void cube_release()
{
    waveform_t *waveform = default_waveform;
    while (waveform)
    {
        waveform_t *next = waveform->next;
        soundio_outstream_destroy(waveform->outstream);
        free(waveform->samples);
        free(waveform);
        waveform = next;
    }
    default_waveform = NULL;

    record_t *record = default_record;
    while (record)
    {
        record_t *next = record->next;
        soundio_instream_destroy(record->instream);
        if (record->ring_buffer)
            soundio_ring_buffer_destroy(record->ring_buffer);
        record->ring_buffer = NULL;
        free(record);
        record = next;
    }
    default_record = NULL;

    if (in_device)
        soundio_device_unref(in_device);
    if (out_device)
        soundio_device_unref(out_device);
    if (soundio)
        soundio_destroy(soundio);

    in_device = NULL;
    out_device = NULL;
    soundio = NULL;
}

EXPORTED cube_native_var *listOutput()
{
    cube_native_var *ret = NATIVE_NULL();
    if (!soundio)
        return ret;

    TO_NATIVE_DICT(ret);

    int count = soundio_output_device_count(soundio);
    int default_device = soundio_default_output_device_index(soundio);

    for (int i = 0; i < count; i++)
    {
        struct SoundIoDevice *device = soundio_get_output_device(soundio, i);
        cube_native_var *dev = NATIVE_DICT();

        if (!device->probe_error)
        {
            ADD_NATIVE_DICT(dev, "isDefault", NATIVE_BOOL(i == default_device));
            ADD_NATIVE_DICT(dev, "isRaw", NATIVE_BOOL(device->is_raw));

            if (device->current_layout.channel_count > 0)
                ADD_NATIVE_DICT(dev, "currentLayout", get_layout_var(&device->current_layout));
            cube_native_var *layouts = NATIVE_LIST();
            for (int i = 0; i < device->layout_count; i++)
            {
                ADD_NATIVE_LIST(layouts, get_layout_var(&device->layouts[i]));
            }
            ADD_NATIVE_DICT(dev, "layouts", layouts);

            if (device->sample_rate_current)
                ADD_NATIVE_DICT(dev, "currentSampleRate", NATIVE_NUMBER(device->sample_rate_current));
            cube_native_var *sample_rates = NATIVE_LIST();
            for (int i = 0; i < device->sample_rate_count; i += 1)
            {
                struct SoundIoSampleRateRange *range = &device->sample_rates[i];
                cube_native_var *sample_rate = NATIVE_LIST();
                ADD_NATIVE_LIST(sample_rate, NATIVE_NUMBER(range->min));
                ADD_NATIVE_LIST(sample_rate, NATIVE_NUMBER(range->max));

                ADD_NATIVE_LIST(sample_rates, sample_rate);
            }
            ADD_NATIVE_DICT(dev, "sampleRates", sample_rates);

            if (device->current_format != SoundIoFormatInvalid)
                ADD_NATIVE_DICT(dev, "currentFormat",
                                NATIVE_STRING_COPY(soundio_format_string(device->current_format)));
            cube_native_var *formats = NATIVE_LIST();
            for (int i = 0; i < device->format_count; i += 1)
            {
                ADD_NATIVE_LIST(formats, NATIVE_STRING_COPY(soundio_format_string(device->formats[i])));
            }
            ADD_NATIVE_DICT(dev, "formats", formats);

            if (device->software_latency_current != 0.0)
                ADD_NATIVE_DICT(dev, "currentSoftwareLatency", NATIVE_NUMBER(device->software_latency_current));
            ADD_NATIVE_DICT(dev, "minSoftwareLatency", NATIVE_NUMBER(device->software_latency_min));
            ADD_NATIVE_DICT(dev, "maxSoftwareLatency", NATIVE_NUMBER(device->software_latency_max));
        }
        ADD_NATIVE_DICT(ret, device->name, dev);
    }

    return ret;
}

EXPORTED cube_native_var *listInput()
{
    cube_native_var *ret = NATIVE_NULL();
    if (!soundio)
        return ret;

    TO_NATIVE_DICT(ret);

    int count = soundio_input_device_count(soundio);
    int default_device = soundio_default_input_device_index(soundio);

    for (int i = 0; i < count; i++)
    {
        struct SoundIoDevice *device = soundio_get_input_device(soundio, i);
        cube_native_var *dev = NATIVE_DICT();

        if (!device->probe_error)
        {
            ADD_NATIVE_DICT(dev, "isDefault", NATIVE_BOOL(i == default_device));
            ADD_NATIVE_DICT(dev, "isRaw", NATIVE_BOOL(device->is_raw));

            if (device->current_layout.channel_count > 0)
                ADD_NATIVE_DICT(dev, "currentLayout", get_layout_var(&device->current_layout));
            cube_native_var *layouts = NATIVE_LIST();
            for (int i = 0; i < device->layout_count; i++)
            {
                ADD_NATIVE_LIST(layouts, get_layout_var(&device->layouts[i]));
            }
            ADD_NATIVE_DICT(dev, "layouts", layouts);

            if (device->sample_rate_current)
                ADD_NATIVE_DICT(dev, "currentSampleRate", NATIVE_NUMBER(device->sample_rate_current));
            cube_native_var *sample_rates = NATIVE_LIST();
            for (int i = 0; i < device->sample_rate_count; i += 1)
            {
                struct SoundIoSampleRateRange *range = &device->sample_rates[i];
                cube_native_var *sample_rate = NATIVE_LIST();
                ADD_NATIVE_LIST(sample_rate, NATIVE_NUMBER(range->min));
                ADD_NATIVE_LIST(sample_rate, NATIVE_NUMBER(range->max));

                ADD_NATIVE_LIST(sample_rates, sample_rate);
            }
            ADD_NATIVE_DICT(dev, "sampleRates", sample_rates);

            if (device->current_format != SoundIoFormatInvalid)
                ADD_NATIVE_DICT(dev, "currentFormat",
                                NATIVE_STRING_COPY(soundio_format_string(device->current_format)));
            cube_native_var *formats = NATIVE_LIST();
            for (int i = 0; i < device->format_count; i += 1)
            {
                ADD_NATIVE_LIST(formats, NATIVE_STRING_COPY(soundio_format_string(device->formats[i])));
            }
            ADD_NATIVE_DICT(dev, "formats", formats);

            if (device->software_latency_current != 0.0)
                ADD_NATIVE_DICT(dev, "currentSoftwareLatency", NATIVE_NUMBER(device->software_latency_current));
            ADD_NATIVE_DICT(dev, "minSoftwareLatency", NATIVE_NUMBER(device->software_latency_min));
            ADD_NATIVE_DICT(dev, "maxSoftwareLatency", NATIVE_NUMBER(device->software_latency_max));
        }
        ADD_NATIVE_DICT(ret, device->name, dev);
    }

    return ret;
}

EXPORTED bool setInput(cube_native_var *device)
{
    char *name = AS_NATIVE_STRING(device);
    int index = get_input_by_name(name);
    if (index < 0)
    {
        if (!is_int(name))
            return false;

        index = atoi(name);
    }

    return set_input_device(index);
}

EXPORTED bool setOutput(cube_native_var *device)
{
    char *name = AS_NATIVE_STRING(device);
    int index = get_output_by_name(name);
    if (index < 0)
    {
        if (!is_int(name))
            return false;

        index = atoi(name);
    }

    return set_output_device(index);
}

EXPORTED bool isPlaying(int id)
{
    if (id < 0)
    {
        waveform_t *waveform = default_waveform;
        while (waveform)
        {
            if (!waveform->done)
                return true;

            waveform = waveform->next;
        }
        return false;
    }
    else
    {
        waveform_t *waveform = get_waveform(id);
        if (!waveform)
            return false;
        return !waveform->done;
    }
}

EXPORTED bool isRecording(int id)
{
    if (id < 0)
    {
        record_t *record = default_record;
        while (record)
        {
            if (!record->done)
                return true;

            record = record->next;
        }
        return false;
    }
    else
    {
        record_t *record = get_record(id);
        if (!record)
            return false;
        return !record->done;
    }
}

EXPORTED int play(cube_native_var *samples, int sample_rate)
{
    if (!out_device)
        return -1;

    if (!IS_NATIVE_LIST(samples))
        return -1;

    if (sample_rate == 0)
        sample_rate = default_out_sample_rate;

    struct SoundIoOutStream *outstream = soundio_outstream_create(out_device);
    if (!outstream)
        return -1;

    int err;
    outstream->format = SoundIoFormatFloat32NE;
    outstream->write_callback = write_callback;
    outstream->sample_rate = sample_rate;

    waveform_t *waveform = (waveform_t *)malloc(sizeof(waveform_t));
    waveform->id = waveform_id++;
    waveform->index = 0;
    waveform->count = samples->size;
    waveform->samples = (float *)malloc(samples->size * sizeof(float));
    for (int i = 0; i < samples->size; i++)
        waveform->samples[i] = (float)AS_NATIVE_NUMBER(samples->list[i]);

    outstream->userdata = waveform;
    waveform->outstream = outstream;
    waveform->done = false;

    waveform->next = default_waveform;
    default_waveform = waveform;

    if ((err = soundio_outstream_open(outstream)))
    {
        soundio_outstream_destroy(outstream);
        return -1;
    }

    if (outstream->layout_error)
    {
        soundio_outstream_destroy(outstream);
        return -1;
    }

    if ((err = soundio_outstream_start(outstream)))
    {
        soundio_outstream_destroy(outstream);
        return -1;
    }

    soundio_flush_events(soundio);

    return waveform->id;
}

EXPORTED int record(float seconds, int sample_rate)
{
    if (!in_device)
        return -1;

    if (sample_rate == 0)
        sample_rate = default_in_sample_rate;

    struct SoundIoInStream *instream = soundio_instream_create(in_device);
    if (!instream)
        return -1;

    int err;
    instream->format = SoundIoFormatFloat32NE;
    instream->sample_rate = sample_rate;
    instream->read_callback = read_callback;

    record_t *record = (record_t *)malloc(sizeof(record_t));
    record->id = record_id++;

    instream->userdata = record;
    record->instream = instream;
    record->done = false;

    record->next = default_record;
    default_record = record;

    if ((err = soundio_instream_open(instream)))
        return -1;

    int capacity = seconds * instream->sample_rate * instream->bytes_per_frame;
    record->ring_buffer = soundio_ring_buffer_create(soundio, capacity);

    if (!record->ring_buffer)
    {
        soundio_instream_destroy(instream);
        return -1;
    }

    if ((err = soundio_instream_start(instream)))
    {
        soundio_instream_destroy(instream);
        return -1;
    }

    return record->id;
}

EXPORTED cube_native_var *read(int id)
{
    record_t *record = get_record(id);
    cube_native_var *ret = NATIVE_NULL();
    if (record == NULL)
        return ret;

    TO_NATIVE_LIST(ret);

    int fill_bytes = soundio_ring_buffer_fill_count(record->ring_buffer);
    float *read_buf = (float *)soundio_ring_buffer_read_ptr(record->ring_buffer);
    int n = (fill_bytes / sizeof(float)) / 2;

    for (int i = 0; i < n; i += record->instream->layout.channel_count)
    {
        ADD_NATIVE_LIST(ret, NATIVE_NUMBER(read_buf[i]));
    }

    soundio_ring_buffer_advance_read_ptr(record->ring_buffer, fill_bytes);

    return ret;
}

EXPORTED cube_native_var *draw(cube_native_var *samples, int width, int height)
{
    if (!samples || !IS_NATIVE_LIST(samples))
        return NULL;

    if (width <= 20 || height <= 20)
        return NULL;

    int size = width * height * 3;

    int sx = 10;
    int sy = height / 2;
    double rx = (width - 20) / (double)samples->size;
    double ry = ((height / (double)2) - 10);

    int stride = width * 3;

    int x, y, index;
    double v;

    unsigned char *data = (unsigned char *)malloc(sizeof(unsigned char) * size);
    memset(data, 255, size);

    for (int i = 0; i < samples->size; i++)
    {
        v = AS_NATIVE_NUMBER(samples->list[i]);

        x = sx + (i * rx);
        y = sy - (v * ry);

        if (y < sy)
        {
            for (int j = y; j < sy; j++)
            {
                index = j * stride + x * 3;
                data[index] = 0;
                data[index + 1] = 163;
                data[index + 2] = 14;
            }
        }
        else
        {
            for (int j = sy; j < y; j++)
            {
                index = j * stride + x * 3;
                data[index] = 0;
                data[index + 1] = 117;
                data[index + 2] = 10;
            }
        }
    }

    return NATIVE_BYTES_COPY(size, data);
}