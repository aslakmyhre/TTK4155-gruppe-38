# Registers and Bitmasks (SAM3X8E)

## Memory-mapped peripherals and pointers

Hardware peripherals have **registers at fixed memory addresses**. The SAM3X8E header files define pointers to structures that describe these registers.

```c
TC2->TC_BMR  // Block Mode Register in Timer/Counter 2
```

- `TC2` is a pointer to the timer's register block.
- `->` accesses a struct member through a pointer (`ptr->field` means `(*ptr).field`).
- Register definitions are normally `volatile`, since hardware can change their values independently of the CPU.

```
Memory address       Contents
────────────────────────────────
0x40080000           Control register
0x40080004           Status register
0x40080008           Counter register
...
```
When the CPU is reading or writing to these registers its actually accessing the peripheri devices registers, not RAM.

### More on Pointers

Often we use some kind of `base addresse` + `offset` to get to the desired address. Lets take this example

```c
TC2->TC_BMR = 0x30; // set TC_BMR to 0x30
```

Here the compiler finds the address of `TC_BMR` in this fasion

$$
\begin{align*}
\texttt{Addresse} &= \texttt{Base Addresse + Offset}\\
&=\texttt{0x40080000 + 0x0C} \\
&=\texttt{0x4008000C}
\end{align*}
$$

Hence, the CPU goes to address `0x4008000C` and sets the value to `0x30`. The way the compiler knows the offset is by inspecting the struct in the original code, e.g.

```c
typedef struct {
    uint32_t TC_CCR;   // 4 bytes
    uint32_t TC_CMR;   // 4 bytes
    uint32_t TC_CV;    // 4 bytes
    uint32_t TC_BMR;   // 4 bytes
} Tc;
```

Here we see that `TC_BMR` is **12** bytes into the struct `Tc`. An important distinction is that `TC_BMR` in itself is not a pointer or a variable containing the offset. Its the placement of the field `TC_BMR` in the struct that decides the offset.

### Some Final Remarks

The **C-code** specifies what is to be done, but the exact machine instructions depend on the compiler. For instance the compiler could just pre-calculate the address for `TC2->TC_BMR` into the machine code. Another way is to have it as two different values in, but then probably use hardware functions to go to offset address.


In this example
```c
void write_register(Tc *peripheral)
{
    peripheral->TC_BMR = 0x30;
}
```
The compiler might not know the `peripheral` addresss before first run of the program, therefore the second method described above probably would be implemented into the machine instructions.

## Bitmasks

Named constants in the device headers select individual bits or bit fields:

```c
// From node2/encoder.c:
TC2->TC_BMR = TC_BMR_QDEN | TC_BMR_POSEN | TC_BMR_EDGPHA;
```

`|` is **bitwise OR**: it combines masks so several features can be enabled in one register write. For example, `0b00010000 | 0b00100000` gives `0b00110000` (illustrative values, not the actual masks).

## Common register operations

```c
reg  = mask;         // Replace the entire register value
reg |= mask;         // Set selected bits; preserve other bits
reg &= ~mask;        // Clear selected bits; preserve other bits
reg ^= mask;         // Toggle selected bits

1u << 5              // Bit 5 = 0b00100000 (bits are numbered from 0)
```

`|=` performs a **read–modify–write**, unlike `=`. Use it only when the register's documented behavior permits it; some hardware registers have special write semantics.

**A dominant `0` can override the recessive** by driving `CAN_H` and `CAN_L` apart. When its sending a recessive `1`, essentially the bus is in its passive condition. So its actually the electrical construction of the transceivers in CAN that makes it possible to use this kind of arbitration.

## Other examples from our encoder code

```c
PMC->PMC_PCER1 = 1u << (ID_TC6 - 32);  // Enable TC6 peripheral clock
PIOC->PIO_ABSR |= PIO_PC25 | PIO_PC26; // Select peripheral function B
int32_t count = (int32_t)TC2->TC_CHANNEL[0].TC_CV; // Read encoder count
```

**Key idea:** The pointer selects *where* to access hardware; the register member selects *what* to configure; the bitmask selects *which bits* to change.