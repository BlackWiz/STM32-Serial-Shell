/** @file em-cli.c
 *
 * @brief Command Line Interface parser implementation for embedded systems.
 *
 * Provides command registration, parsing, and execution without dynamic
 * memory allocation. Designed for bare-metal UART communication.
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2025 Your Name. All rights reserved.
 */

#include "em-cli.h"
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* String constants for error messages */
static char const CLI_MSG_INCORRECT_PARAMS[] =
    "Incorrect command parameter(s). Enter \"help\" to view commands.\r\n\r\n";
static char const CLI_MSG_NOT_RECOGNIZED[] =
    "Command not recognized. Enter 'help' to view commands.\r\n\r\n";
static char const CLI_MSG_HELP_HEADER[] = "Available commands:\r\n";
static char const CLI_MSG_MISSING_PARAM[] = "Error: Missing parameter\r\n";

/* Character constants for parsing */
#define CHAR_SPACE          ' '
#define CHAR_NULL           '\0'
#define CHAR_CARRIAGE_RET   '\r'
#define CHAR_NEWLINE        '\n'

/* Command registry globals */
cli_command_definition_t g_commands_array[CLI_MAX_COMMANDS];
int32_t g_command_count = 0;
char g_cli_write_buffer[CLI_WRITE_BUFFER_SIZE];

/* Built-in command definitions */
const cli_command_definition_t g_help_command = {
    "help",
    "\r\nhelp:\r\nLists all registered commands\r\n",
    cli_help_interpreter,
    -1
};

const cli_command_definition_t g_set_command = {
    "set",
    "\r\nset <key> <value>:\r\nSets a key-value pair\r\n",
    cli_set_interpreter,
    2
};

const cli_command_definition_t g_get_command = {
    "get",
    "\r\nget <key>:\r\nGets a value by key\r\n",
    cli_get_interpreter,
    1
};


/*!
 * @brief Register a new command in the CLI system.
 *
 * @param[in] p_command_to_register Pointer to command definition structure.
 *
 * @return CLI_TRUE on success, CLI_FALSE if registry is full or NULL pointer.
 */
base_type
cli_register_command (cli_command_definition_t const * const p_command_to_register)
{
    base_type is_registered = CLI_FALSE;

    if ((NULL != p_command_to_register) &&
        (g_command_count < CLI_MAX_COMMANDS))
    {
        g_commands_array[g_command_count] = *p_command_to_register;
        g_command_count++;
        is_registered = CLI_TRUE;
    }

    return is_registered;
}


/*!
 * @brief Process a received command string.
 *
 * Parses command input, validates parameters, and calls registered handler.
 *
 * @param[in] p_command_input Pointer to null-terminated command string.
 * @param[out] p_write_buffer Buffer for command response output.
 * @param[in] write_buffer_len Size of write buffer in bytes.
 *
 * @return CLI_FALSE when command processing is complete, CLI_TRUE if more output pending.
 */
base_type
cli_process_command (char const * const p_command_input,
                     char * p_write_buffer,
                     size_t write_buffer_len)
{
    base_type is_processed = CLI_PASS;
    char const * p_registered_command = NULL;
    base_type command_length = 0;
    static int32_t command_index = 0;
    base_type param_count = 0;

    if ((NULL == p_command_input) || (NULL == p_write_buffer))
    {
        return CLI_FALSE;
    }

    /* Search for matching command on first call */
    if (0 == command_index)
    {
        for (command_index = 0; command_index < g_command_count; command_index++)
        {
            p_registered_command = g_commands_array[command_index].p_command;
            command_length = (base_type)strlen(p_registered_command);

            /* Check if command matches and has proper delimiter */
            if ((CHAR_SPACE == p_command_input[command_length]) ||
                (CHAR_NULL == p_command_input[command_length]) ||
                (CHAR_CARRIAGE_RET == p_command_input[command_length]) ||
                (CHAR_NEWLINE == p_command_input[command_length]))
            {
                if (0 == strncmp(p_command_input,
                                 p_registered_command,
                                 (size_t)command_length))
                {
                    /* Command found - validate parameter count */
                    if (g_commands_array[command_index].expected_parameter_count >= 0)
                    {
                        param_count = cli_get_parameter_count(p_command_input);

                        if (param_count != g_commands_array[command_index].expected_parameter_count)
                        {
                            is_processed = CLI_FALSE;
                        }
                    }
                    break;
                }
            }
        }
    }

    if (CLI_FALSE == is_processed)
    {
        /* Incorrect parameter count */
        (void)strncpy(p_write_buffer,
                      CLI_MSG_INCORRECT_PARAMS,
                      write_buffer_len);
    }
    else if ((CLI_PASS == is_processed) && (command_index < g_command_count))
    {
        /* Call registered command handler */
        is_processed = g_commands_array[command_index].p_command_interpreter(
            p_write_buffer,
            write_buffer_len,
            p_command_input);

        /* Reset for next command if processing complete */
        if (CLI_FALSE == is_processed)
        {
            command_index = 0;
        }
    }
    else
    {
        /* Command not found */
        (void)strncpy(p_write_buffer,
                      CLI_MSG_NOT_RECOGNIZED,
                      write_buffer_len);
        is_processed = CLI_FALSE;
    }

    return is_processed;
}


/*!
 * @brief Help command handler - lists all registered commands.
 *
 * @param[out] p_write_buffer Output buffer for help text.
 * @param[in] write_buffer_len Size of output buffer.
 * @param[in] p_command_string Original command string (unused).
 *
 * @return CLI_FALSE (command complete).
 */
base_type
cli_help_interpreter (char * p_write_buffer,
                      size_t write_buffer_len,
                      char const * const p_command_string)
{
    int32_t cmd_index = 0;
    size_t current_len = 0u;
    size_t cmd_name_len = 0u;
    char const * p_cmd_name = NULL;

    (void)p_command_string; /* Unused parameter */

    if (NULL == p_write_buffer)
    {
        return CLI_FALSE;
    }

    /* Initialize buffer with header */
    (void)memset(p_write_buffer, 0, write_buffer_len);
    (void)strncpy(p_write_buffer, CLI_MSG_HELP_HEADER, write_buffer_len);
    current_len = strlen(p_write_buffer);

    /* List all registered commands except help itself */
    for (cmd_index = 0; cmd_index < g_command_count; cmd_index++)
    {
        if (0 == cmd_index)
        {
            continue; /* Skip help command */
        }

        p_cmd_name = g_commands_array[cmd_index].p_command;
        cmd_name_len = strlen(p_cmd_name);

        /* Check buffer space before adding (need space for "  \r\n" + null) */
        if ((current_len + cmd_name_len + 5u) < write_buffer_len)
        {
            (void)strncat(p_write_buffer,
                          "  ",
                          write_buffer_len - current_len - 1u);
            (void)strncat(p_write_buffer,
                          p_cmd_name,
                          write_buffer_len - current_len - 1u);
            (void)strncat(p_write_buffer,
                          "\r\n",
                          write_buffer_len - current_len - 1u);
            current_len = strlen(p_write_buffer);
        }
    }

    return CLI_FALSE;
}


/*!
 * @brief Set command handler - sets a key-value pair.
 *
 * @param[out] p_write_buffer Output buffer for response.
 * @param[in] write_buffer_len Size of output buffer.
 * @param[in] p_command_string Original command string with parameters.
 *
 * @return CLI_FALSE (command complete).
 */
base_type
cli_set_interpreter (char * p_write_buffer,
                     size_t write_buffer_len,
                     char const * const p_command_string)
{
    char const * p_param = NULL;
    base_type param_len = 0;
    char param1[50];
    char param2[50];

    if ((NULL == p_write_buffer) || (NULL == p_command_string))
    {
        return CLI_FALSE;
    }

    (void)memset(p_write_buffer, 0, write_buffer_len);

    /* Extract first parameter (key) */
    p_param = cli_get_parameter(p_command_string, 1, &param_len);

    if ((NULL != p_param) && (param_len < 50))
    {
        (void)strncpy(param1, p_param, (size_t)param_len);
        param1[param_len] = CHAR_NULL;
    }
    else
    {
        (void)strncpy(p_write_buffer,
                      CLI_MSG_MISSING_PARAM,
                      write_buffer_len);
        return CLI_FALSE;
    }

    /* Extract second parameter (value) */
    p_param = cli_get_parameter(p_command_string, 2, &param_len);

    if ((NULL != p_param) && (param_len < 50))
    {
        (void)strncpy(param2, p_param, (size_t)param_len);
        param2[param_len] = CHAR_NULL;
    }
    else
    {
        (void)strncpy(p_write_buffer,
                      CLI_MSG_MISSING_PARAM,
                      write_buffer_len);
        return CLI_FALSE;
    }

    /* Format response */
    (void)sprintf(p_write_buffer, "Set %s = %s\r\n", param1, param2);

    return CLI_FALSE;
}


/*!
 * @brief Get command handler - retrieves a value by key.
 *
 * @param[out] p_write_buffer Output buffer for response.
 * @param[in] write_buffer_len Size of output buffer.
 * @param[in] p_command_string Original command string with parameters.
 *
 * @return CLI_FALSE (command complete).
 */
base_type
cli_get_interpreter (char * p_write_buffer,
                     size_t write_buffer_len,
                     char const * const p_command_string)
{
    char const * p_param = NULL;
    base_type param_len = 0;
    char param[50];

    if ((NULL == p_write_buffer) || (NULL == p_command_string))
    {
        return CLI_FALSE;
    }

    (void)memset(p_write_buffer, 0, write_buffer_len);

    /* Extract parameter (key) */
    p_param = cli_get_parameter(p_command_string, 1, &param_len);

    if ((NULL != p_param) && (param_len < 50))
    {
        (void)strncpy(param, p_param, (size_t)param_len);
        param[param_len] = CHAR_NULL;
        (void)sprintf(p_write_buffer,
                      "Get %s: [value not implemented]\r\n",
                      param);
    }
    else
    {
        (void)strncpy(p_write_buffer,
                      CLI_MSG_MISSING_PARAM,
                      write_buffer_len);
    }

    return CLI_FALSE;
}


/*!
 * @brief Count parameters in a command string.
 *
 * @param[in] p_command_string Pointer to command string.
 *
 * @return Number of space-delimited parameters (excluding command itself).
 */
base_type
cli_get_parameter_count (char const * p_command_string)
{
    base_type param_count = 0;
    base_type is_prev_space = CLI_FALSE;

    if (NULL == p_command_string)
    {
        return 0;
    }

    /* Count space-delimited words */
    while ((CHAR_NULL != *p_command_string) &&
           (CHAR_CARRIAGE_RET != *p_command_string) &&
           (CHAR_NEWLINE != *p_command_string))
    {
        if (CHAR_SPACE == *p_command_string)
        {
            if (CLI_PASS != is_prev_space)
            {
                param_count++;
                is_prev_space = CLI_PASS;
            }
        }
        else
        {
            is_prev_space = CLI_FALSE;
        }

        p_command_string++;
    }

    /* Adjust if command ended with spaces */
    if (CLI_PASS == is_prev_space)
    {
        param_count--;
    }

    return param_count;
}


/*!
 * @brief Extract a specific parameter from command string.
 *
 * @param[in] p_command_string Pointer to command string.
 * @param[in] wanted_parameter Parameter index (1-based).
 * @param[out] p_parameter_length Pointer to store parameter length.
 *
 * @return Pointer to parameter start, or NULL if not found.
 */
char const *
cli_get_parameter (char const * p_command_string,
                   base_type wanted_parameter,
                   base_type * p_parameter_length)
{
    base_type params_found = 0;
    char const * p_return_param = NULL;

    if ((NULL == p_command_string) || (NULL == p_parameter_length))
    {
        return NULL;
    }

    *p_parameter_length = 0;

    while (params_found < wanted_parameter)
    {
        /* Skip current word */
        while ((CHAR_NULL != *p_command_string) &&
               (CHAR_SPACE != *p_command_string) &&
               (CHAR_CARRIAGE_RET != *p_command_string) &&
               (CHAR_NEWLINE != *p_command_string))
        {
            p_command_string++;
        }

        /* Skip spaces */
        while ((CHAR_NULL != *p_command_string) &&
               (CHAR_SPACE == *p_command_string))
        {
            p_command_string++;
        }

        /* Check if we found a valid string */
        if ((CHAR_NULL != *p_command_string) &&
            (CHAR_CARRIAGE_RET != *p_command_string) &&
            (CHAR_NEWLINE != *p_command_string))
        {
            params_found++;

            if (params_found == wanted_parameter)
            {
                /* Found target parameter - measure its length */
                p_return_param = p_command_string;

                while ((CHAR_NULL != *p_command_string) &&
                       (CHAR_SPACE != *p_command_string) &&
                       (CHAR_CARRIAGE_RET != *p_command_string) &&
                       (CHAR_NEWLINE != *p_command_string))
                {
                    (*p_parameter_length)++;
                    p_command_string++;
                }

                if (0 == *p_parameter_length)
                {
                    p_return_param = NULL;
                }

                break;
            }
        }
        else
        {
            break;
        }
    }

    return p_return_param;
}

#ifdef __cplusplus
}
#endif

/*** end of file ***/