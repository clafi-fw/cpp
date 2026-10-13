# Diagnostic

The words that do not fit above a declaration.

## Options

`enabled` is the global switch. Every other option is set in `Options::Stated` and read through
its function - `logThemeCrossing()`, `apiErrors()` and the rest - which answers off, or
`ApiErrors::Ignore`, while `enabled` is off.

**Off means not built**: code behind an option sits in an `if constexpr`. Turning one on is a
rebuild.

**A release defines `CLAFI_NO_DIAGNOSTIC`**, which holds `enabled` off whatever the source says -
`-p:ClaFiNoDiagnostic=true` for Visual Studio, `-DCLAFI_NO_DIAGNOSTIC=ON` for CMake.

## diagnosticLog

Writes a numbered line to the Output page of the diagnostic window and to the debugger's output.
Lines are dropped while the pointer is over the page, which then reads Stopped.

**The diagnostic window** holds the Output and FPS pages, with an Always on top check beside the
tabs where the platform can keep a window on top. The application builds it at startup and shows
it if the config says it was up. Show diagnostic in the application menu shows it, and its close
button hides it.

## createDiagnosticLogConfigSchema

The `Diagnostic` section, which ApplicationBase adds to every application's schema:

- `Visible` - whether the window was up.
- `Tab` - the page it was on, `Output` or `FPS`. Any other value reads as Output.

The window's placement is kept under `Diagnostic` in the Forms section, as the main form's is.

**Both are in every build**, with Diagnostic::Options::enabled off as well, so a config file reads
the same whichever build wrote it.

## storeDiagnosticLogState

Called by ApplicationBase::saveConfig. It writes `Tab` as well - see
createDiagnosticLogConfigSchema.

**The placement is written here while the window is up**: a form writes its placement when it
hides, and a window that is up would otherwise be saved where it last hid.

## FrameExtent

- `Whole` - counts toward the rate and states a paint time.
- `Partial` - counts toward the rate only. What a part cost says nothing about the whole.

## FpsPage

It shows the window's facts and what its frames cost - paint time, draw rate, frame rate and CPU
share - each against its extremes. A chart shows the rate over the last two seconds.

**The page drives its target.** While it shows, the window under the pointer is asked for a whole
frame whenever the thread is idle, and each paint is timed. This spends a core.

- A moving pointer holds the drive off, and the paints it causes are then what is counted.
- A paint covering under 80% of the window counts toward the rate only - see FrameExtent.
- The target is read off the pointer on every tick. Another window starts the measurement over.
- Over this page's own window, or over no framework window, nothing is driven and the readings
  stay.

**The Backend row is the application's**, the same for every window, so it shows before anything
is measured.

## FpsChart

**Two curves**: the filled area is the rate frames arrived at, the line over it the rate the
drawing alone would allow. They meet when the drawing limits the rate. They part when something
after the drawing does - the rest of the form, the copy to the screen, or the display server's
pace.

**The chart reads its benchmark and never runs it.** Whoever runs the benchmark repaints the chart.

**Colours** are the theme's accent for the curve and spot for the field, read at each paint. A
chart over something with a palette of its own states them at full strength - ChartFieldColor and
ChartSeriesColor, or setColors. The chart sets the alpha either way.

## BenchmarkValue

The mean is kept without holding the readings.

**Before the first reading**, lastValue and minValue answer k_maxFloat and maxValue a negative.
timesNum is zero until then, and whoever shows a reading decides what to show instead.

## CpuUsage

A percentage of one core: the CPU time the process used between two samples, over the wall time
between them. The platform takes the reading - see Platform::processCpuTime.

## FpsBucket

A slice is 10 ms of FpsBenchmark's history. It holds totals, so it costs the same however many
frames land in it.

## FpsBenchmark

**The first frame of a run states nothing** - no interval and no paint time. It starts the clock
the next interval is measured from.

`buckets()` holds the last two seconds as slices, which is what FpsChart draws.
