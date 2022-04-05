from tkinter import Canvas
from tkinter import Tk
from tkinter import ttk
import time

def create_animation_window():
    window = Tk()
    window.title("N-body Visualization")
    # Disabled window size configuration and create a default-sized window
    # Resize the window later with Canvas
    # window.columnconfigure(0, weight = 1)
    # window.rowconfigure(0, weight = 1)
    return window

def create_animation_canvas(window):
    canvas = Canvas(window, bg='black', height=1000, width=1900, bd=1)
    canvas.pack()
    canvas.create_oval(100, 100, 105, 105, fill='green')
    return canvas

def animate(canvas, window, width, height):
    iter = 0
    prev_iter = 0
    # Currently using sample viz data
    # TODO: Parameterize file name for viz data
    with open("viz_data_25_25.txt") as f:
        for line in f:
            line_str = line.split()
            numbers_float = [float(x) for x in line_str]
            iter = numbers_float[0]
            mass = numbers_float[1]
            x = numbers_float[2] * 400
            y = numbers_float[3] * 300

            if iter != prev_iter:
                time.sleep(1)
                canvas.delete("all")
                prev_iter = iter

            if mass >= 8:
                oval_color = 'red'
                oval_radius = 12
            elif mass >= 6:
                oval_color = 'yellow'
                oval_radius = 8
            elif mass >= 4:
                oval_color = 'white'
                oval_radius = 6
            else:
                oval_color = 'green'
                oval_radius = 4
            canvas.create_oval(width/2 - x, height/2 - y, width/2 - x + oval_radius, height/2 - y + oval_radius, fill=oval_color)
            window.update()

# Create a visualization window with a Canvas inside of it
# Call animate() to initialize the animation
viz_window = create_animation_window()
viz_canvas = create_animation_canvas(viz_window)
animate(viz_canvas, viz_window, 1900, 1000)