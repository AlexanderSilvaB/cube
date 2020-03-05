#include "linenoise.hpp"
#include <cube/cubeext.h>

std::vector<std::string> completion_words;

extern "C"
{
    EXPORTED void linenoise_init()
    {
        linenoise::SetCompletionCallback([](const char *editBuffer, std::vector<std::string> &completions) {
            int index = 0;
            int len = strlen(editBuffer);

            for (int i = len - 1; i > 0; i--)
            {
                if (editBuffer[i] == ' ' || editBuffer[i] == '[' || editBuffer[i] == '(' || editBuffer[i] == '=' ||
                    editBuffer[i] == '<' || editBuffer[i] == '>' || editBuffer[i] == '/')
                {
                    index = i + 1;
                    break;
                }
            }

            if (index >= len)
                return;

            std::string back(editBuffer, index);
            std::string front(editBuffer, index, len);

            for (int i = 0; i < completion_words.size(); i++)
            {
                if (strncmp(completion_words[i].c_str(), front.c_str(), front.size()) == 0)
                {
                    completions.push_back(back + completion_words[i]);
                }
            }
        });
    }

    EXPORTED void linenoise_set_multiline(bool multiLineMode)
    {
        linenoise::SetMultiLine(multiLineMode);
    }

    EXPORTED bool linenoise_load_history(const char *path)
    {
        return linenoise::LoadHistory(path);
    }

    EXPORTED bool linenoise_save_history(const char *path)
    {
        return linenoise::SaveHistory(path);
    }

    EXPORTED bool linenoise_add_history(const char *line)
    {
        return linenoise::AddHistory(line);
    }

    EXPORTED bool linenoise_set_history_max_len(size_t len)
    {
        return linenoise::SetHistoryMaxLen(len);
    }

    EXPORTED bool linenoise_read_line(const char *prompt, char *line)
    {
        std::string _line;
        bool quit = linenoise::Readline(prompt, _line);
        if (!quit)
            strcpy(line, _line.c_str());
        return quit;
    }

    EXPORTED void linenoise_add_keyword(const char *keyword)
    {
        completion_words.push_back(keyword);
    }

    EXPORTED void linenoise_add_keyword_2(const char *keyword, const char *property)
    {
        completion_words.push_back(std::string(keyword) + "." + std::string(property));
    }
}
