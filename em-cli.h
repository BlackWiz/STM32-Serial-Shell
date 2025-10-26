/** @file em-cli.h
 *
 * @brief Command Line Interface parser for embedded UART communication.
 *
 * Provides command registration, parsing, and execution framework for
 * interactive UART-based command processing on bare-metal systems.
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2025 Your Name. All rights reserved.
 */

#ifndef EM_CLI_H
#define EM_CLI_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Base type definitions */
typedef int32_t base_type;

/* Boolean constants */
#define CLI_FALSE             ((base_type)0)
#define CLI_TRUE              ((base_type)1)
#define CLI_PASS              ((base_type)1)

/* Maximum number of commands that can be registered */
#define CLI_MAX_COMMANDS      10u

/* Maximum buffer size for command responses */
#define CLI_WRITE_BUFFER_SIZE 512u

/* CRITICAL: Use array-based registration (no dynamic allocation) */
#define CLI_ARRAY_BASED_REGISTER

/**
 * @brief Command definition structure.
 *
 * Defines a single CLI command with its handler function and metadata.
 */
typedef struct command_line_input
{
    char const * p_command;                 /**< Command string (e.g., "set") */
    char const * p_help_string;             /**< Help text for this command */
    base_type (*p_command_interpreter)(     /**< Command handler function */
        char * p_write_buffer,
        size_t write_buffer_len,
        char const * const p_command_string);
    int8_t expected_parameter_count;        /**< Required parameter count (-1 = variable) */
} cli_command_definition_t;

/* Global command registry array */
extern cli_command_definition_t g_commands_array[CLI_MAX_COMMANDS];
extern int32_t g_command_count;
extern char g_cli_write_buffer[CLI_WRITE_BUFFER_SIZE];

/* Built-in command definitions */
extern const cli_command_definition_t g_help_command;
extern const cli_command_definition_t g_set_command;
extern const cli_command_definition_t g_get_command;

/* Public API functions */

/*!
 * @brief Register a new command in the CLI system.
 *
 * @param[in] p_command_to_register Pointer to command definition structure.
 *
 * @return CLI_TRUE on success, CLI_FALSE if registry is full.
 */
base_type cli_register_command(cli_command_definition_t const * const p_command_to_register);

/*!
 * @brief Process a received command string.
 *
 * @param[in] p_command_input Pointer to null-terminated command string.
 * @param[out] p_write_buffer Buffer for command response output.
 * @param[in] write_buffer_len Size of write buffer in bytes.
 *
 * @return CLI_FALSE when command processing is complete, CLI_TRUE if more output pending.
 */
base_type cli_process_command(char const * const p_command_input,
                               char * p_write_buffer,
                               size_t write_buffer_len);

/*!
 * @brief Count parameters in a command string.
 *
 * @param[in] p_command_string Pointer to command string.
 *
 * @return Number of space-delimited parameters (excluding command itself).
 */
base_type cli_get_parameter_count(char const * p_command_string);

/*!
 * @brief Extract a specific parameter from command string.
 *
 * @param[in] p_command_string Pointer to command string.
 * @param[in] wanted_parameter Parameter index (1-based).
 * @param[out] p_parameter_length Pointer to store parameter length.
 *
 * @return Pointer to parameter start, or NULL if not found.
 */
char const * cli_get_parameter(char const * p_command_string,
                                base_type wanted_parameter,
                                base_type * p_parameter_length);

/* Built-in command handler prototypes */

/*!
 * @brief Help command handler - lists all registered commands.
 *
 * @param[out] p_write_buffer Output buffer for help text.
 * @param[in] write_buffer_len Size of output buffer.
 * @param[in] p_command_string Original command string (unused).
 *
 * @return CLI_FALSE (command complete).
 */
base_type cli_help_interpreter(char * p_write_buffer,
                                size_t write_buffer_len,
                                char const * const p_command_string);

/*!
 * @brief Set command handler - sets a key-value pair.
 *
 * @param[out] p_write_buffer Output buffer for response.
 * @param[in] write_buffer_len Size of output buffer.
 * @param[in] p_command_string Original command string with parameters.
 *
 * @return CLI_FALSE (command complete).
 */
base_type cli_set_interpreter(char * p_write_buffer,
                               size_t write_buffer_len,
                               char const * const p_command_string);

/*!
 * @brief Get command handler - retrieves a value by key.
 *
 * @param[out] p_write_buffer Output buffer for response.
 * @param[in] write_buffer_len Size of output buffer.
 * @param[in] p_command_string Original command string with parameters.
 *
 * @return CLI_FALSE (command complete).
 */
base_type cli_get_interpreter(char * p_write_buffer,
                               size_t write_buffer_len,
                               char const * const p_command_string);

#ifdef __cplusplus
}
#endif

#endif /* EM_CLI_H */

/*** end of file ***/