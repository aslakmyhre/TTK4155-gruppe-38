#ifndef MCP2515_TEST_H
#define MCP2515_TEST_H

/* Exercises every MCP2515 instruction and a loopback frame, one result per
   OLED line (0-6). Leaves the controller reset, in configuration mode. */
void mcp2515_test(void);

#endif
