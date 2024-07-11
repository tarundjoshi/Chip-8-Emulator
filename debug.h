void print_debug_info(chip8_t *chip8) {
    printf("Address: 0x%04X, Opcode: 0x%04X Desc: ",
           chip8->PC-2, chip8->inst.opcode);

    switch ((chip8->inst.opcode >> 12) & 0x0F) {
        case 0x00:
            if (chip8->inst.NN == 0xE0) {
                // 0x00E0: Clear the screen
                printf("Clear screen\n");

            } else if (chip8->inst.NN == 0xEE) {
                // 0x00EE: Return from subroutine
                // Set program counter to last address on subroutine stack ("pop" it off the stack)
                //   so that next opcode will be gotten from that address.
                printf("Return from subroutine to address 0x%04X\n",
                       *(chip8->stack_ptr - 1));
            } else {
                printf("Unimplemented Opcode.\n");
            }
            break;

        case 0x01:
            // 0x1NNN: Jump to address NNN
            printf("Jump to address NNN (0x%04X)\n",
                   chip8->inst.NNN);   
            break;

        case 0x02:
            // 0x2NNN: Call subroutine at NNN
            // Store current address to return to on subroutine stack ("push" it on the stack)
            //   and set program counter to subroutine address so that the next opcode
            //   is gotten from there.
            printf("Call subroutine at NNN (0x%04X)\n",
                   chip8->inst.NNN);
            break;

        case 0x03:
            // 0x3XNN: Check if VX == NN, if so, skip the next instruction
            printf("Check if V%X (0x%02X) == NN (0x%02X), skip next instruction if true\n",
                   chip8->inst.X, chip8->V[chip8->inst.X], chip8->inst.NN);
            break;

        case 0x04:
            // 0x4XNN: Check if VX != NN, if so, skip the next instruction
            printf("Check if V%X (0x%02X) != NN (0x%02X), skip next instruction if true\n",
                   chip8->inst.X, chip8->V[chip8->inst.X], chip8->inst.NN);
            break;

        case 0x05:
            // 0x5XY0: Check if VX == VY, if so, skip the next instruction
            printf("Check if V%X (0x%02X) == V%X (0x%02X), skip next instruction if true\n",
                   chip8->inst.X, chip8->V[chip8->inst.X], 
                   chip8->inst.Y, chip8->V[chip8->inst.Y]);
            break;

        case 0x06:
            // 0x6XNN: Set register VX to NN
            printf("Set register V%X = NN (0x%02X)\n",
                   chip8->inst.X, chip8->inst.NN);
            break;

        case 0x07:
            // 0x7XNN: Set register VX += NN
            printf("Set register V%X (0x%02X) += NN (0x%02X). Result: 0x%02X\n",
                   chip8->inst.X, chip8->V[chip8->inst.X], chip8->inst.NN,
                   chip8->V[chip8->inst.X] + chip8->inst.NN);
            break;

        case 0x08:
            switch(chip8->inst.N) {
                case 0:
                    // 0x8XY0: Set register VX = VY
                    printf("Set register V%X = V%X (0x%02X)\n",
                           chip8->inst.X, chip8->inst.Y, chip8->V[chip8->inst.Y]);
                    break;

                case 1:
                    // 0x8XY1: Set register VX |= VY
                    printf("Set register V%X (0x%02X) |= V%X (0x%02X); Result: 0x%02X\n",
                           chip8->inst.X, chip8->V[chip8->inst.X],
                           chip8->inst.Y, chip8->V[chip8->inst.Y],
                           chip8->V[chip8->inst.X] | chip8->V[chip8->inst.Y]);
                    break;

                case 2:
                    // 0x8XY2: Set register VX &= VY
                    printf("Set register V%X (0x%02X) &= V%X (0x%02X); Result: 0x%02X\n",
                           chip8->inst.X, chip8->V[chip8->inst.X],
                           chip8->inst.Y, chip8->V[chip8->inst.Y],
                           chip8->V[chip8->inst.X] & chip8->V[chip8->inst.Y]);
                    break;

                case 3:
                    // 0x8XY3: Set register VX ^= VY
                    printf("Set register V%X (0x%02X) ^= V%X (0x%02X); Result: 0x%02X\n",
                           chip8->inst.X, chip8->V[chip8->inst.X],
                           chip8->inst.Y, chip8->V[chip8->inst.Y],
                           chip8->V[chip8->inst.X] ^ chip8->V[chip8->inst.Y]);
                    break;

                case 4:
                    // 0x8XY4: Set register VX += VY, set VF to 1 if carry
                    printf("Set register V%X (0x%02X) += V%X (0x%02X), VF = 1 if carry; Result: 0x%02X, VF = %X\n",
                           chip8->inst.X, chip8->V[chip8->inst.X],
                           chip8->inst.Y, chip8->V[chip8->inst.Y],
                           chip8->V[chip8->inst.X] + chip8->V[chip8->inst.Y],
                           ((uint16_t)(chip8->V[chip8->inst.X] + chip8->V[chip8->inst.Y]) > 255));
                    break;

                case 5:
                    // 0x8XY5: Set register VX -= VY, set VF to 1 if there is not a borrow (result is positive/0)
                    printf("Set register V%X (0x%02X) -= V%X (0x%02X), VF = 1 if no borrow; Result: 0x%02X, VF = %X\n",
                           chip8->inst.X, chip8->V[chip8->inst.X],
                           chip8->inst.Y, chip8->V[chip8->inst.Y],
                           chip8->V[chip8->inst.X] - chip8->V[chip8->inst.Y],
                           (chip8->V[chip8->inst.Y] <= chip8->V[chip8->inst.X]));
                    break;

                case 6:
                    // 0x8XY6: Set register VX >>= 1, store shifted off bit in VF
                    printf("Set register V%X (0x%02X) >>= 1, VF = shifted off bit (%X); Result: 0x%02X\n",
                           chip8->inst.X, chip8->V[chip8->inst.X],
                           chip8->V[chip8->inst.X] & 1,
                           chip8->V[chip8->inst.X] >> 1);
                    break;

                case 7:
                    // 0x8XY7: Set register VX = VY - VX, set VF to 1 if there is not a borrow (result is positive/0)
                    printf("Set register V%X = V%X (0x%02X) - V%X (0x%02X), VF = 1 if no borrow; Result: 0x%02X, VF = %X\n",
                           chip8->inst.X, chip8->inst.Y, chip8->V[chip8->inst.Y],
                           chip8->inst.X, chip8->V[chip8->inst.X],
                           chip8->V[chip8->inst.Y] - chip8->V[chip8->inst.X],
                           (chip8->V[chip8->inst.X] <= chip8->V[chip8->inst.Y]));
                    break;

                case 0xE:
                    // 0x8XYE: Set register VX <<= 1, store shifted off bit in VF
                    printf("Set register V%X (0x%02X) <<= 1, VF = shifted off bit (%X); Result: 0x%02X\n",
                           chip8->inst.X, chip8->V[chip8->inst.X],
                           (chip8->V[chip8->inst.X] & 0x80) >> 7,
                           chip8->V[chip8->inst.X] << 1);
                    break;

                default:
                    // Wrong/unimplemented opcode
                    break;
            }
            break;

        case 0x09:
            // 0x9XY0: Check if VX != VY; Skip next instruction if so
            printf("Check if V%X (0x%02X) != V%X (0x%02X), skip next instruction if true\n",
                   chip8->inst.X, chip8->V[chip8->inst.X], 
                   chip8->inst.Y, chip8->V[chip8->inst.Y]);
            break;

        case 0x0A:
            // 0xANNN: Set index register I to NNN
            printf("Set I to NNN (0x%04X)\n",
                   chip8->inst.NNN);
            break;

        case 0x0B:
            // 0xBNNN: Jump to V0 + NNN
            printf("Set PC to V0 (0x%02X) + NNN (0x%04X); Result PC = 0x%04X\n",
                   chip8->V[0], chip8->inst.NNN, chip8->V[0] + chip8->inst.NNN);
            break;

        case 0x0C:
            // 0xCXNN: Sets register VX = rand() % 256 & NN (bitwise AND)
            printf("Set V%X = rand() %% 256 & NN (0x%02X)\n",
                   chip8->inst.X, chip8->inst.NN);
            break;

        case 0x0D:
            // 0xDXYN: Draw N-height sprite at coords X,Y; Read from memory location I;
            //   Screen pixels are XOR'd with sprite bits, 
            //   VF (Carry flag) is set if any screen pixels are set off; This is useful
            //   for collision detection or other reasons.
            printf("Draw N (%u) height sprite at coords V%X (0x%02X), V%X (0x%02X) "
                   "from memory location I (0x%04X). Set VF = 1 if any pixels are turned off.\n",
                   chip8->inst.N, chip8->inst.X, chip8->V[chip8->inst.X], chip8->inst.Y,
                   chip8->V[chip8->inst.Y], chip8->I);
            break;

        case 0x0E:
            if (chip8->inst.NN == 0x9E) {
                // 0xEX9E: Skip next instruction if key in VX is pressed
                printf("Skip next instruction if key in V%X (0x%02X) is pressed; Keypad value: %d\n",
                       chip8->inst.X, chip8->V[chip8->inst.X], chip8->keypad[chip8->V[chip8->inst.X]]);

            } else if (chip8->inst.NN == 0xA1) {
                // 0xEX9E: Skip next instruction if key in VX is not pressed
                printf("Skip next instruction if key in V%X (0x%02X) is not pressed; Keypad value: %d\n",
                       chip8->inst.X, chip8->V[chip8->inst.X], chip8->keypad[chip8->V[chip8->inst.X]]);
            }
            break;

        case 0x0F:
            switch (chip8->inst.NN) {
                case 0x0A:
                    // 0xFX0A: VX = get_key(); Await until a keypress, and store in VX
                    printf("Await until a key is pressed; Store key in V%X\n",
                           chip8->inst.X);
                    break;

                case 0x1E:
                    // 0xFX1E: I += VX; Add VX to register I. For non-Amiga CHIP8, does not affect VF
                    printf("I (0x%04X) += V%X (0x%02X); Result (I): 0x%04X\n",
                           chip8->I, chip8->inst.X, chip8->V[chip8->inst.X],
                           chip8->I + chip8->V[chip8->inst.X]);
                    break;

                case 0x07:
                    // 0xFX07: VX = delay timer
                    printf("Set V%X = delay timer value (0x%02X)\n",
                           chip8->inst.X, chip8->delay_timer);
                    break;

                case 0x15:
                    // 0xFX15: delay timer = VX 
                    printf("Set delay timer value = V%X (0x%02X)\n",
                           chip8->inst.X, chip8->V[chip8->inst.X]);
                    break;

                case 0x18:
                    // 0xFX18: sound timer = VX 
                    printf("Set sound timer value = V%X (0x%02X)\n",
                           chip8->inst.X, chip8->V[chip8->inst.X]);
                    break;

                case 0x29:
                    // 0xFX29: Set register I to sprite location in memory for character in VX (0x0-0xF)
                    printf("Set I to sprite location in memory for character in V%X (0x%02X). Result(VX*5) = (0x%02X)\n",
                           chip8->inst.X, chip8->V[chip8->inst.X], chip8->V[chip8->inst.X] * 5);
                    break;

                case 0x33:
                    // 0xFX33: Store BCD representation of VX at memory offset from I;
                    //   I = hundred's place, I+1 = ten's place, I+2 = one's place
                    printf("Store BCD representation of V%X (0x%02X) at memory from I (0x%04X)\n",
                           chip8->inst.X, chip8->V[chip8->inst.X], chip8->I);
                    break;

                case 0x55:
                    // 0xFX55: Register dump V0-VX inclusive to memory offset from I;
                    //   SCHIP does not inrement I, CHIP8 does increment I
                    printf("Register dump V0-V%X (0x%02X) inclusive at memory from I (0x%04X)\n",
                           chip8->inst.X, chip8->V[chip8->inst.X], chip8->I);
                    break;

                case 0x65:
                    // 0xFX65: Register load V0-VX inclusive from memory offset from I;
                    //   SCHIP does not inrement I, CHIP8 does increment I
                    printf("Register load V0-V%X (0x%02X) inclusive at memory from I (0x%04X)\n",
                           chip8->inst.X, chip8->V[chip8->inst.X], chip8->I);
                    break;

                default:
                    break;
            }
            break;
            
        default:
            printf("Unimplemented Opcode.\n");
            break;  // Unimplemented or invalid opcode
    }
}