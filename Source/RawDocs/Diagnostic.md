# Diagnostic

The words that no longer fit above a declaration.

## DiagnosticLog

The diagnostic window is a window like any other: a title bar, the Output and FPS pages under it,
a tab strip along the bottom with the Always on top check beside the tabs. It is built with the
application and hidden until the application menu's Show diagnostic, which brings it to the front
when it is already up; its close button hides it, and the application's exit takes it down.

WHAT IT REMEMBERS is kept in the config in two places. Its placement stands in the Forms
section under `Diagnostic`, the way the main form's does - always on top included where the
platform keeps that, which is Win32. Whether it was up and which page it was on stand in a
section of its own, `Diagnostic`, as `Visible` and `Tab`;
createDiagnosticLogConfigSchema states that section and ApplicationBase adds it to every
application's schema. restoreDiagnosticLogState reads both once the config is loaded, opens
that page and shows the window where it says so. storeDiagnosticLogState writes them, and the
placement while the window is up, from ApplicationBase::saveConfig - the window is up for as
long as the application, so the hide that writes a placement never comes for it, and the
config is saved before the window goes.

THE PAGE IS KEPT BY ITS TAB CAPTION rather than by a number, so the config file says `Output`
or `FPS` and a page added between them moves nothing. The captions are stated once and the
tabs are built from them, so the name written and the name matched cannot come apart, and a
name that matches neither reads as the Output page. The constructor selects no tab at all -
which page the window opens on is the config's answer, and restoreDiagnosticLogState is the
first moment that answer exists.

The Always on top check reads the window's state on every state query and toggles it on a click,
so it never holds a state of its own. Where IPlatformWindow::canSetAlwaysOnTop answers no the
check is hidden.

## FrameExtent

How much of what is measured a frame covered. EVERY FRAME COUNTS TOWARDS THE RATE;
ONLY A WHOLE ONE STATES A PAINT TIME. A paint of a part is an arrival like any other, but
what it cost says nothing about what the whole costs.

## FpsPage

What the framework window under the pointer paints: what the window is, what a
frame of it costs and the rate its paints arrive at, each reading against its extremes,
and the rate charted over the last two seconds.

THE PAGE DRIVES ITS TARGET. While the page is showing, a repeater asks the window
under the pointer for a whole frame on every tick, and each paint of that window comes
back through FormPaintedEvent and is timed. The repeater is a timer, which is the lowest
priority message there is: a pointer streaming moves, and the paints they cause, hold it
off for as long as the pointer keeps moving. So the whole frames are the idle thread's
work, and a core is spent on them for as long as the page is open with a window under the
pointer.

EVERY PAINT COUNTS TOWARDS THE RATE; ONLY A WHOLE ONE STATES A PAINT TIME. A paint
covering less than 80% of the window is a control repainting itself - a tile the pointer
has reached - which is an arrival like any other and says nothing about what the whole
costs.

THE TARGET IS READ OFF THE POINTER ON EVERY TICK and never followed across one. Over
this page's own window, or over no framework window, there is none: nothing is driven and
the readings stand at their last values. A window that has gone is never reached, because
the one pointer kept to it is compared and never dereferenced - what the window grid says
about it was copied while it was live.

THE BACKEND ROW IS THE APPLICATION'S ANSWER AND NOT THE TARGET'S. One context builds every
window of an application, so they all draw through the same kind and this page's own form answers
for them - which is what lets that row stand while nothing is being measured and every other row
reads a dash.

THE NUMBERS ARE TEXT CELLS OF MOVING COLUMNS - see Grids::MovingText. Rewritten ten
times a second, they are shaped past the layout cache, which a text cell of an ordinary
column would cycle and empty for every other window.

## FpsChart

The frame rate a benchmark measured over the last two seconds, drawn against an
axis that names the rate.

TWO CURVES, and the gap between them is the point. The filled one is the rate frames
actually arrived at; the line above it is the rate the drawing alone would allow. They
meet when the drawing is what limits the rate, and part when something after the drawing
is - the rest of the form, the copy to the screen, or the display server's own pace.

THE CHART READS ITS BENCHMARK AND NEVER RUNS IT. Whoever runs the benchmark says when
the chart is repainted; the chart holds no timer and asks for no frame of its own.

ITS COLOURS ARE STATED where it stands over something with a palette of its own - a
chart over a scene belongs to that scene's. Given none, it takes the theme's: the accent for
the curve and the spot for the field, resolved on whatever it stands on at each paint, so a
theme switch carries it across. Either way the two are full strength, and the weight each
is drawn at is the chart's own.

## BenchmarkValue

A running aggregate of one measured quantity - the last reading, the extremes it has
passed through, and a mean kept without holding the readings it was taken over.

An aggregate that has measured nothing answers k_maxFloat for its last and its least
reading, and a negative for its greatest. Whoever shows a reading before the first
measurement is the one that has to say what it shows instead.

## CpuUsage

The share of one core this process is taking, as a percentage, sampled no oftener
than once a second.

The platform answers with the CPU time the process has consumed since it started, and
the share is the step in that time over the wall time between two samples. Nothing here
knows how the reading is taken - see Platform::processCpuTime.

## FpsBucket

The frames measured inside one slice of the history, held as their totals so a
slice costs the same whatever number of frames lands in it.

## FpsBenchmark

Times a piece of work, keeping both a running aggregate of it and the last two
seconds of it in slices - which is what a chart of the rate over time is drawn from.
