
```
VERSION = 0.1.5
BINNAME = easygcoder
WORKDIR = /etc/easygcoder
SVG_M = 10                            # Scale
USR = serg

TOOL_D = 0.2                          # Tool diameter for milling operations
TOOL_CH_HEIGHT = 100                  # Tool change height

SVG_WIDTH = 1920                      # SVG work field width
SVG_HEIGHT = 1080                     # SVG work field height
SVG_MARGIN = 50                       # Gap in pixels between model and work field boundaries

DRILL_SAFE = 5                        # safe height of lifting the tool above the work surface
DRILL_START = 0                       # The position from which the working stroke of the tool begins at the working feed rate
DRILL_DEEP = -2.4                     # The depth at which the drilling cycle will be stopped
DRILL_STEP = -0.6                     # The drilling step depth. The tool will be inserted to this depth, after which it will be withdrawn to a safe height. The cycle then repeats, but starting from a new position.
DRILL_FEED = 50                       # drill feed rate during drilling

MILLING_SAFE = 5                      # safe height of lifting the tool above the work surface
MILLING_START = 0.1                   # The position from which the working stroke of the tool begins at the working feed rate
MILLING_DEEP = -0.1
MILLING_STEP = -0.2
MILLING_FEED = 100                    # milling feed rate

CONFIGPATH = /etc/easygcoder/easygcoder.conf

```

The number of steps is calculated based on the starting position, depth and step size.
