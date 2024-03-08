"""
Calculates the number of steps the 13x13 BB Champion
3SICAAAKSAAEUCIBEACQRCAIGUEhBAgIQQAgGWgABCFoiQAAYWoCAAACCA
runs for.
"""
import decimal

def sweep_steps(sweep_len, initial):
    return sweep_len * 20 + (43 if initial else 41)

def glider_steps(counter):
    return counter * 36 - 13

def final_sweep_steps(sweep_len, sweep_exit):
    return 26 + 16 * (sweep_len - sweep_exit) + 9

# Bootstrap steps (total steps until entry of first glider loop)
INI_STEPS = 14

# Initial glider loop counter
INI_COUNT = 5

steps = INI_STEPS
glider_count = INI_COUNT
sweep_len = 1
sweep_exit = None

while True:
    steps += glider_steps(glider_count)
    glider_count *= 3
    sweep_val = glider_count - 1
    if sweep_exit:
        pending_iterations -= 1
        if pending_iterations == 0:
            break
    else:
        if sweep_val % 7 == 0 and sweep_len > 1:
            sweep_exit = sweep_len + 1
            pending_iterations = sweep_val / 7

    steps += sweep_steps(sweep_len, sweep_len == 1)
    sweep_len += 1

steps += final_sweep_steps(sweep_len, sweep_exit)
print(steps)
print(format(decimal.Decimal(steps), '.3e'))