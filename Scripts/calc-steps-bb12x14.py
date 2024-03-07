"""
Calculates the number of steps the 12x14 BB Champion
zggIAACkgAEUCCEAAAEQgAEQEoGUAEECAmWQIEIgAEAWiEAAGAgCAAAIIA
runs for.
"""
import decimal

# The number of steps from glider loop exit @9,6
# until before glider loop entry @6,7
def sweep_steps(sweep_len):
    return sweep_len * 20 + 37

def glider_steps(counter, initial):
    return counter * 42 - (11 if initial else 15)

def final_sweep_steps(sweep_len, sweep_exit):
    return 23 + 16 * (sweep_len - sweep_exit + 1) + 9

# Bootstrap steps (total steps until entry of first glider loop)
INI_STEPS = 13

# Initial glider loop counter
INI_COUNT = 5

steps = INI_STEPS
glider_count = INI_COUNT
sweep_len = 1
sweep_exit = None

while True:
    steps += glider_steps(glider_count, glider_count == INI_COUNT)
    glider_count *= 3
    sweep_val = glider_count - 1
    if sweep_exit:
        pending_iterations -= 1
        if pending_iterations == 0:
            break
    else:
        if sweep_val % 7 == 0 and sweep_len > 1:
            sweep_exit = sweep_len + 1
            pending_iterations = sweep_val / 7 - 1

    steps += sweep_steps(sweep_len)
    sweep_len += 1

steps += final_sweep_steps(sweep_len, sweep_exit)
print(steps)
print(format(decimal.Decimal(steps), '.3e'))