/** @file main.c
 *
 * @brief Main application entry point for CLI-UART bridge.
 *
 * Implements command-line interface over UART for interactive control
 * of embedded system. Receives commands, parses them, executes handlers,
 * and transmits responses back via UART.
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2025 Your Name. All rights reserved.
 */

#include <stdint.h>
#include <string.h>
#include "uart.h"
#include "em-cli.h"

#ifdef __cplusplus
extern "C" {
#endif

/* NVIC register base address */
#define NVIC_ISER0_BASE     0xE000E100u

/* USART2 interrupt number */
#define USART2_IRQ_NUM      28u

/* String constants */
static char const WELCOME_MSG[] = "\r\nCLI Ready. Type 'help' for commands.\r\n> ";
static char const PROMPT[] = "> ";

/* External UART state variables */
extern volatile uart_state_t g_tx_state;
extern volatile uart_state_t g_rx_state;
extern char * g_p_rx_buffer;
extern volatile uint32_t g_rx_index;


/*!
 * @brief Enable global interrupts.
 */
static inline void
enable_global_irq (void)
{
    __asm volatile ("cpsie i" : : : "memory");
}


/*!
 * @brief Enable specific NVIC interrupt.
 *
 * @param[in] irq_num Interrupt number to enable.
 */
static inline void
nvic_enable_irq (uint32_t const irq_num)
{
    volatile uint32_t * p_nvic_iser = (volatile uint32_t *)NVIC_ISER0_BASE;
    p_nvic_iser[irq_num >> 5u] = (1u << (irq_num & 0x1Fu));
}


/*!
 * @brief Wait for UART transmitter to become idle.
 */
static void
wait_for_tx_idle (void)
{
    while (UART_STATE_IDLE != g_tx_state)
    {
        /* Busy wait */
    }
}


/*!
 * @brief Initialize CLI subsystem.
 *
 * Configures UART peripheral, registers built-in commands,
 * enables interrupts, and displays welcome message.
 */
static void
cli_init (void)
{
    /* Initialize UART peripheral */
    (void)uart_init();

    /* Register built-in commands */
    (void)cli_register_command(&g_help_command);
    (void)cli_register_command(&g_set_command);
    (void)cli_register_command(&g_get_command);

    /* Enable USART2 interrupt in NVIC */
    nvic_enable_irq(USART2_IRQ_NUM);
    enable_global_irq();

    /* Transmit welcome message */
    wait_for_tx_idle();
    (void)uart_transmit_buffer(WELCOME_MSG);

    /* Start listening for first command */
    (void)uart_receive_buffer();
}


/*!
 * @brief Process received CLI commands.
 *
 * Called continuously in main loop. When a complete command has been
 * received via UART, parses it, executes the handler, transmits the
 * response, and restarts reception.
 */
static void
cli_process (void)
{
    static char response_buffer[CLI_WRITE_BUFFER_SIZE];

    /* Only process when command reception is complete */
    if (UART_STATE_IDLE == g_rx_state)
    {
        /* Null-terminate received command */
        if (g_rx_index > 0u)
        {
            g_p_rx_buffer[g_rx_index] = '\0';
        }

        /* Parse and execute command */
        (void)cli_process_command(g_p_rx_buffer,
                                  response_buffer,
                                  sizeof(response_buffer));

        /* Wait for transmitter to be ready */
        wait_for_tx_idle();

        /* Send command response */
        (void)uart_transmit_buffer(response_buffer);

        /* Wait for response transmission to complete */
        wait_for_tx_idle();

        /* Send command prompt */
        (void)uart_transmit_buffer(PROMPT);

        /* Reset receive buffer index */
        g_rx_index = 0u;

        /* Restart reception for next command */
        (void)uart_receive_buffer();
    }
}


/*!
 * @brief Main application entry point.
 *
 * Initializes CLI subsystem and enters infinite command processing loop.
 *
 * @return Never returns (embedded system main loop).
 */
int32_t
main (void)
{
    cli_init();

    for (;;)
    {
        cli_process();
    }

    /* Never reached */
    return 0;
}

#ifdef __cplusplus
}
#endif

/*** end of file ***/