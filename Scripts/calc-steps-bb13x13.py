"""
Calculates the total number of steps the 13x13 BB Champion
3SICAAAKSAAEUCIBEAAARSAkGUEiBBgIQQIgGWgABCFogQAAYkoCAFgCCA
Steps = 2.882 * 10^23409

Old chamption:
3SICAAAKSAAEUCIBEACQRCAIGUEhBAgIQQAgGWgABCFoiQAAYWoCAAACCA
Steps = 3.6098 * 10^750
"""
import decimal
import itertools

def sweep_steps(sweep_len, initial):
    return sweep_len * 20 + (45 if initial else 41)

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
    sweep_val = glider_count * 3 - 1
    if sweep_len < 10:
        print(sweep_len, sweep_val)
    glider_count *= 5
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

print(sweep_len)

steps += final_sweep_steps(sweep_len, sweep_exit)
str_steps = str(steps)
while len(str_steps):
    print(str_steps[:70])
    str_steps = str_steps[70:]
print(format(decimal.Decimal(steps), '.3e'))