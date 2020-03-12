#include <cube/cubeext.h>
#include <curl/curl.h>

#define VERBOSE 0L
#define NOPROGRESS 1L

// Callbacks
static size_t write_data_path(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    cube_native_bytes *bytes = (cube_native_bytes *)stream;
    size_t offset = bytes->length;
    size_t len = size * nmemb;
    bytes->length += len;
    bytes->bytes = (unsigned char *)realloc(bytes->bytes, bytes->length);
    memcpy(bytes->bytes + offset, ptr, len);
    return len;
}

size_t read_data(void *ptr, size_t size, size_t nmemb, void *userp)
{
    cube_native_bytes *data = (cube_native_bytes *)userp;

    size_t byte_len = size * nmemb;
    if (data->length < byte_len)
    {
        byte_len = data->length;
    }
    memcpy(ptr, data->bytes, byte_len);
    data->bytes += byte_len;
    data->length -= byte_len;
    return byte_len;
}

// Lib
typedef struct Session_t
{
    int id;
    CURL *curl;
    cube_native_bytes write_bytes, read_bytes;
    struct Session_t *next;
} Session;

static int session_id = 0;
Session *current = NULL;

Session *create_session()
{
    Session *session = (Session *)malloc(sizeof(Session));
    session->curl = curl_easy_init();
    if (!session->curl)
    {
        free(session);
        return NULL;
    }

    session->id = session_id++;
    session->read_bytes.bytes = NULL;
    session->read_bytes.length = 0;
    session->write_bytes.bytes = NULL;
    session->write_bytes.length = 0;
    session->next = current;
    current = session;
    return session;
}

Session *get_session(int id)
{
    Session *cur = current;
    while (cur != NULL)
    {
        if (cur->id == id)
            return cur;
        cur = cur->next;
    }
    return NULL;
}

void finish_session(Session *session)
{
    if (session == current)
    {
        current = session->next;
        curl_easy_cleanup(session->curl);
        session->read_bytes.bytes = realloc(session->read_bytes.bytes, 0);
        session->write_bytes.bytes = realloc(session->write_bytes.bytes, 0);
        session->read_bytes.bytes = NULL;
        session->read_bytes.length = 0;
        session->write_bytes.bytes = NULL;
        session->write_bytes.length = 0;
        free(session);
        return;
    }
    Session *cur = current;
    while (cur->next != NULL)
    {
        if (cur->next == session)
        {
            cur->next = session->next;
            curl_easy_cleanup(session->curl);
            session->read_bytes.bytes = realloc(session->read_bytes.bytes, 0);
            session->write_bytes.bytes = realloc(session->write_bytes.bytes, 0);
            session->read_bytes.bytes = NULL;
            session->read_bytes.length = 0;
            session->write_bytes.bytes = NULL;
            session->write_bytes.length = 0;
            free(session);
            return;
        }
        cur = cur->next;
    }
}

EXPORTED void cube_init()
{
    curl_global_init(CURL_GLOBAL_ALL);
}

EXPORTED void cube_release()
{
    Session *cur;
    while (current != NULL)
    {
        cur = current->next;
        curl_easy_cleanup(current->curl);
        current->read_bytes.bytes = realloc(current->read_bytes.bytes, 0);
        current->write_bytes.bytes = realloc(current->write_bytes.bytes, 0);
        current->read_bytes.bytes = NULL;
        current->read_bytes.length = 0;
        current->write_bytes.bytes = NULL;
        current->write_bytes.length = 0;
        free(current);
        current = cur;
    }
    current = NULL;
    curl_global_cleanup();
}

EXPORTED cube_native_var *download(cube_native_var *url, cube_native_var *path)
{
    bool success = false;

    CURL *curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, AS_NATIVE_STRING(url));
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, VERBOSE);
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, NOPROGRESS);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data_path);
    FILE *file = fopen(AS_NATIVE_STRING(path), "wb");
    if (file)
    {
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, file);
        success = curl_easy_perform(curl_handle) == CURLE_OK;
        fclose(file);
    }
    curl_easy_cleanup(curl_handle);
    return NATIVE_BOOL(success);
}

EXPORTED cube_native_var *get(cube_native_var *url)
{
    cube_native_var *result = NATIVE_NONE();
    CURL *curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, AS_NATIVE_STRING(url));
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, VERBOSE);
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, NOPROGRESS);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
    cube_native_bytes bytes;
    bytes.bytes = NULL;
    bytes.length = 0;
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &bytes);
    if (curl_easy_perform(curl_handle) == CURLE_OK)
    {
        TO_NATIVE_BYTES_ARG(result, bytes.length, bytes.bytes);
    }
    curl_easy_cleanup(curl_handle);
    return result;
}

EXPORTED cube_native_var *post(cube_native_var *url, cube_native_var *data_var)
{
    cube_native_var *result = NATIVE_NONE();
    CURL *curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, AS_NATIVE_STRING(url));
    curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, VERBOSE);
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, NOPROGRESS);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);

    cube_native_bytes orig_data = AS_NATIVE_BYTES(data_var);
    cube_native_bytes data;
    data.bytes = orig_data.bytes;
    data.length = orig_data.length;
    curl_easy_setopt(curl_handle, CURLOPT_READDATA, &data);
    curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, read_data);
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, data.length);

    cube_native_bytes bytes;
    bytes.bytes = NULL;
    bytes.length = 0;
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &bytes);
    if (curl_easy_perform(curl_handle) == CURLE_OK)
    {
        TO_NATIVE_BYTES_ARG(result, bytes.length, bytes.bytes);
    }
    curl_easy_cleanup(curl_handle);
    return result;
}

EXPORTED cube_native_var *easy_init()
{
    Session *session = create_session();
    if (!session)
        return NATIVE_NONE();
    return NATIVE_NUMBER(session->id);
}

EXPORTED void easy_cleanup(cube_native_var *id)
{
    Session *session = get_session(AS_NATIVE_NUMBER(id));
    if (session != NULL)
    {
        finish_session(session);
    }
}

EXPORTED cube_native_var *easy_setopt(cube_native_var *id, cube_native_var *option, cube_native_var *parameter)
{
    Session *session = get_session(AS_NATIVE_NUMBER(id));
    if (session == NULL)
        return NATIVE_NONE();

    CURLcode res;
    long opt = (long)AS_NATIVE_NUMBER(option);
    if (parameter->type == TYPE_BOOL)
        res = curl_easy_setopt(session->curl, opt, (long)AS_NATIVE_BOOL(parameter));
    else if (parameter->type == TYPE_NUMBER)
        res = curl_easy_setopt(session->curl, opt, (long)AS_NATIVE_NUMBER(parameter));
    else if (parameter->type == TYPE_STRING)
        res = curl_easy_setopt(session->curl, opt, AS_NATIVE_STRING(parameter));
    else if (parameter->type == TYPE_FUNC)
        return NATIVE_NONE();
    else
        return NATIVE_NONE();
    return NATIVE_NUMBER(res);
}

EXPORTED cube_native_var *easy_perform(cube_native_var *id)
{
    Session *session = get_session(AS_NATIVE_NUMBER(id));
    if (session == NULL)
        return NATIVE_NONE();

    session->write_bytes.bytes = realloc(session->write_bytes.bytes, 0);
    session->write_bytes.length = 0;
    curl_easy_setopt(session->curl, CURLOPT_WRITEDATA, &session->write_bytes);
    curl_easy_setopt(session->curl, CURLOPT_WRITEFUNCTION, write_data);

    curl_easy_setopt(session->curl, CURLOPT_READDATA, &session->read_bytes);
    curl_easy_setopt(session->curl, CURLOPT_READFUNCTION, read_data);
    curl_easy_setopt(session->curl, CURLOPT_POSTFIELDSIZE, session->read_bytes.length);

    CURLcode res = curl_easy_perform(session->curl);
    return NATIVE_NUMBER(res);
}

EXPORTED cube_native_var *easy_getinfo(cube_native_var *sid, cube_native_var *info)
{
    cube_native_var *ret = NATIVE_NONE();
    Session *session = get_session(AS_NATIVE_NUMBER(sid));
    if (session == NULL)
        return ret;

    long id = AS_NATIVE_NUMBER(info);
    if (id == CURLINFO_EFFECTIVE_URL || id == CURLINFO_CONTENT_TYPE)
    {
        char *rt;
        CURLcode res = curl_easy_getinfo(session->curl, id, &rt);
        if (res == CURLE_OK && rt)
        {
            TO_NATIVE_STRING(ret, COPY_STR(rt));
        }
    }
    else if (id == CURLINFO_CONNECT_TIME || id == CURLINFO_CONTENT_LENGTH_DOWNLOAD ||
             id == CURLINFO_CONTENT_LENGTH_UPLOAD || id == CURLINFO_NAMELOOKUP_TIME ||
             id == CURLINFO_PRETRANSFER_TIME || id == CURLINFO_REDIRECT_TIME || id == CURLINFO_SIZE_DOWNLOAD ||
             id == CURLINFO_SIZE_UPLOAD || id == CURLINFO_SPEED_DOWNLOAD || id == CURLINFO_SPEED_UPLOAD ||
             id == CURLINFO_STARTTRANSFER_TIME || id == CURLINFO_TOTAL_TIME)
    {
        double rt;
        CURLcode res = curl_easy_getinfo(session->curl, id, &rt);
        if (res == CURLE_OK)
        {
            TO_NATIVE_NUMBER(ret, rt);
        }
    }
    else if (id == CURLINFO_FILETIME)
    {
        long rt;
        CURLcode res = curl_easy_getinfo(session->curl, id, &rt);
        if (res == CURLE_OK)
        {
            TO_NATIVE_NUMBER(ret, rt);
        }
    }
    else
    {
        int rt;
        CURLcode res = curl_easy_getinfo(session->curl, id, &rt);
        if (res == CURLE_OK)
        {
            TO_NATIVE_NUMBER(ret, rt);
        }
    }
    return ret;
}

EXPORTED void set_data(cube_native_var *id, cube_native_var *data)
{
    Session *session = get_session(AS_NATIVE_NUMBER(id));
    if (session == NULL)
        return;

    cube_native_bytes bytes = AS_NATIVE_BYTES(data);
    session->read_bytes.length = bytes.length;
    session->read_bytes.bytes = realloc(session->read_bytes.bytes, bytes.length);
    memcpy(session->read_bytes.bytes, bytes.bytes, bytes.length);
}

EXPORTED cube_native_var *get_data(cube_native_var *id)
{
    Session *session = get_session(AS_NATIVE_NUMBER(id));
    if (session == NULL)
        return NATIVE_NONE();

    cube_native_bytes bytes;
    bytes.length = session->write_bytes.length;
    bytes.bytes = (unsigned char *)malloc(bytes.length);
    memcpy(bytes.bytes, session->write_bytes.bytes, bytes.length);
    return NATIVE_BYTES_ARG(bytes.length, bytes.bytes);
}