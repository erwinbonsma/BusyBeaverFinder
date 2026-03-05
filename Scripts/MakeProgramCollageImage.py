import fileinput
from PIL import Image, ImageDraw

UNIT_SIZE = 32
GRID_SEP = 64
RADIUS = 14
GRID_SIZE = 7
NCOL, NROW = 10, 13


def draw_program(draw_img, col, row, prog_spec):
    x0, y0 = tuple(pos * (UNIT_SIZE * GRID_SIZE + GRID_SEP) + GRID_SEP // 2 for pos in (col, row))

    x_min = x0 + UNIT_SIZE // 2
    x_max = x_min + UNIT_SIZE * (GRID_SIZE - 1)
    y_min = y0 + UNIT_SIZE // 2
    y_max = y_min + UNIT_SIZE * (GRID_SIZE - 1)
    for i in range(GRID_SIZE):
        draw_img.line([(x_min, y_min + i * UNIT_SIZE),
                       (x_max, y_min + i * UNIT_SIZE)],
                      fill='black')
        draw_img.line([(x_min + i * UNIT_SIZE, y_min),
                       (x_min + i * UNIT_SIZE, y_max)],
                      fill='black')

    col, row = (0, 0)
    x0 += UNIT_SIZE // 2
    y0 += UNIT_SIZE // 2
    for ch in prog_spec:
        if ch == '*' or ch == 'o':
            draw_img.circle((x0 + col * UNIT_SIZE, y0 + row * UNIT_SIZE),
                            RADIUS,
                            outline='black',
                            fill='black' if ch == '*' else 'white',
                            width=2),
        elif ch == '.':
            draw_img.circle((x0 + col * UNIT_SIZE, y0 + row * UNIT_SIZE),
                            8,
                            outline='grey',
                            fill='grey',
                            width=2),
        col += 1
        if col == GRID_SIZE:
            col = 0
            row += 1


def make_image():
    img = Image.new('RGB',
                    tuple((UNIT_SIZE * GRID_SIZE + GRID_SEP) * d for d in (NCOL, NROW)),
                    (255, 255, 255))
    col, row = (0, 0)
    draw_img = ImageDraw.Draw(img)
    for prog_spec in fileinput.input():
        draw_program(draw_img, col, row, prog_spec)
        col += 1
        if col == NCOL:
            col = 0
            row += 1
    return img


img = make_image()
img = img.resize((d // 4 for d in img.size))
img.save("ProgramCollage.png")
