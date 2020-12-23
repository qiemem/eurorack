Mutable Instruments Stages multi-mode firmware
==============================================

This is an unofficial firmware for Mutable Instruments Stages. It was originally created by joeSeggiola and started as a modification to let you enter and exit the "Ouroboros" mode (the **harmonic oscillator** easter egg) at runtime, while the module is powered on. Then, it evolved adding the ability to sequence harmonic ratios, enable slower free-running LFOs and providing a completely alternative mode that transforms the module into **six identical DAHDSR envelope generators**.

This fork adds the following to segment generator mode:

- **Bipolar LFOs, steps, and holds**: Toggle a segment's polarity by holding its button and wiggling its pot
- **Very-slow and audio-rate LFOs**: Change an LFO's frequency range by holding its button and moving its slider to top, middle, or bottom
- **Re-trigger control**: hold a ramp segment's button and wiggle its pot to toggle re-trigger behavior; this allows for DUSG/Maths-style clock dividers, subharmonic generators, and more!
- **Fast and slow envelopes**: Change the range of a ramp's slider by holding its button and move the slider to top, middle, or bottom
- **Arbitrarily slow clocked LFOs and improved audio-rate clocked LFOs**
- **Hold and step segment quantization**: Hold button and move slider to select scale; slider transposes within key, allowing selection of mode
- **Start and end value tracking for ramp segments**: This can be used as a kind of VCA or crossfader

See segment generator mode's [usage instructions](#segment-generator) for details. None of these features interfere with the normal workflow of Stages.

This fork also adds a new mode, [advanced segment generator](#advanced-segment-generator), which adds

- **Random segment type**: A fourth type of segment, which gives access to four different random/chaotic algorithms:
    - Uniform random CV (single, non-looping, non-gated)
    - Double-scroll attractor, a smooth chaotic system similar to the Lorenz system (single, looping, non-gated)
    - Emulation of Tom Whitwell's Turing Machine (non-looping, gated)
    - Logistic map, a discrete chaotic system (single, looping, gated)
- **Slew with independent rise and fall times**: Single, non-gated, non-looping ramp (green) segments slew with independent rise and fall: CV is target value, pot is rise, and slider is fall; this can be used as an AR envelope or an envelope follower as well
    - Unipolar and bipolar mode: applies full-wave rectification to incoming signal in unipolar mode for optimal use as an envelope follower
- **Probabilistic gates**: A single, looping, gated red segment's pot now controls its probability of firing, giving you a mini-branches (the pot used to do nothing in this case)
- **Attenuverter segments**: Single, looping step (yellow) segments attenuate instead of slew. Non-looping still slew, so no functionality is lost.

Finally, this fork allows you to control the frequency range of the harmonic oscillator mode (aka ouroboros mode; the Stages easter egg), giving access to 6 harmonically related LFOs.

This fork includes the changes in official firmware 1.1 as well as [the extended sequencer official firmware](https://forum.mutable-instruments.net/t/stages-extended-sequencer-firmware/17493).

This firmware should fully support [chained modules](https://mutable-instruments.net/modules/stages/manual/#chaining-modules)! See [multi-mode usage](#multi-mode-usage).

Check out this great video overview of the fork as well as of the extended sequencer by Peakae (does not include v1.1.0 updates):

[![](https://img.youtube.com/vi/_-rgoM_oNE0/0.jpg)](https://www.youtube.com/watch?v=_-rgoM_oNE0)Check out this great video overview by Peakae (does not include v1.1.0 updates):

Download and installation
-------------------------

📦 Download the **[latest WAV file here][2]** and follow the [firmware update procedure here][3]. Source code is available [here][8]. joeSeggiola's original source code is available [here][9].

IMPORTANT: Installation will clear the module settings if coming from a different firmware. Right after updating from an earlier version of this fork, the stock Stages firmware or joeSeggiola's version, Stages may continuously cycle between green, orange, and red LEDs. Turning the module off and on again should restore functionality. This happens because this fork expands the amount of data stored for each segment, so will be incompatible with the settings stored from a different firmware. If you encounter problems, please let me know, either in a GitHub issue or otherwise.

[2]: https://github.com/qiemem/eurorack/releases/latest
[3]: https://mutable-instruments.net/modules/stages/manual/#firmware
[8]: https://github.com/qiemem/eurorack/tree/stages-multi/stages
[9]: https://github.com/joeSeggiola/eurorack/tree/stages-multi/stages

Cheatsheet
---

- Button + slider: Hold button and move slider to top, middle, or bottom to change value
- Button + pot: Hold button and wiggle pot to toggle

Control overview (modes in order of corresponding button; hold button for 5 seconds to switch):
| Mode             | Button   | Hold button 1s | Slider + CV        | Pot                | Button + slider | Button + pot       |
| ---              | ---      | ---            | ---                | ---                | ---             | ---                |
| Segment gen      | Seg type | Toggle looping | Time / level       | Shape / time       | LFO range (G)   | Polarity / re-trig |
| Adv segment gen  | Seg type | Toggle looping | Time / level       | Shape / time       | LFO range (G)   | Polarity / re-trig |
| Slow LFO         | Seg type | Toggle looping | Time / level       | Shape / time       | LFO range (G)   | Polarity / re-trig |
| DAHDSR           | Gate     | -              | Time / level (5)   | Shape (246)        | -               | -                  |
| Harmonic osc     | Shape    | Toggle shapes  | Freq (1) / amp     | Tune (1) / harmony | Freq range (1)  | -                  |
| Alt harmonic osc | Shape    | Toggle shapes  | Freq (1) / harmony | Tune (1) / amp     | Freq range (1)  | -                  |

Single segment types in segment generator modes (* = advanced mode; all other behaviors are unchanged from original Stages):
| Segment type          | Behavior        | Slider + CV       | Pot         | Button + slider | Button + pot        |
| ---                   | ---             | ---               | ---         | ---             | ---                 |
| Green                 | Zero / R&F slew*| Fall time*        | Rise time*  | -               | Rectification*      |
| Green looping         | LFO             | Freq              | Shape       | Freq range      | Polarity            |
| Green gated           | Decay           | Time              | Shape       | Time range      | Re-trigger behavior |
| Green gated, looping  | Clocked LFO     | Div/mul           | Shape       | Div/mul range   | Polarity            |
| Yellow                | Slew            | Offset            | Time        | Quant scale     | Slider polarity     |
| Yellow looping        | Slew / Atten*   | Offset            | Time / Att* | Quant scale     | Slider polarity     |
| Yellow gated          | S&H             | Offset            | Slew        | Quant scale     | Slider polarity     |
| Yellow gated, looping | S&H / Atten*    | Offset            | Time / Att* | Quant scale     | Slider polarity     |
| Red                   | Delay           | Offset            | Time        | Quant scale     | Slider polarity     |
| Red looping           | Delay           | Offset            | Time        | Quant scale     | Slider polarity     |
| Red gated             | Timed pulse     | Offset            | Pulse width | Quant scale     | Slider polarity     |
| Red gated, looping    | Gate generator  | Offset            | Probability | Quant scale     | Slider polarity     |
| GR*                   | Uniform random  | Freq              | Slew        | Freq range      | Polarity            |
| GR looping*           | Double scroll   | Freq              | Shape       | Freq range      | Polarity            |
| GR gated*             | Turing machine  | Prob of bit flip  | Steps       | -               | Polarity            |
| GR gated, looping*    | Logistic map    | Reproduction rate | Slew        | -               | Polarity            |

Segment types in groups (only green~red and button+pot are different from original Stages):
| Segment type | Behavior       | Slider + CV      | Pot   | Button + slider | Button + pot        |
| ---          | ---            | ---              | ---   | ---             | ---                 |
| Green        | Ramp           | Time             | Shape | Time range*     | Re-trigger behavior |
| Yellow       | Step           | Offset           | Slew  | -               | Slider polarity     |
| Red          | Hold           | Offset           | Time  | -               | Slider polarity     |
| Red looping  | Sustain        | Offset           | -     | -               | Slider polarity     |
| Green~red*   | Turing machine | Prob of bit flip | Steps | -               | Polarity            |

Segment generator LED codes:
- Blink dim red 1/sec: Bipolar / no re-trigger (for ramp)
- Oscillations between color and black: Looping mode
- Speed of green oscillations: LFO freq range
- Solid green followed by ramp down: Ramp time range (default is solid)
- Oscillation between green and red: Random segment type in advanced mode
- Fading in of slider/type LED after button + slider/pot: Value of slider/pot adjusting to position (giving you a chance to change a property without affecting slider/pot value much)

Harmonic oscillator shapes:
- Normal: Sine (green), triangle (yellow), square (red)
- Looping: Ramp (green), small pulse (yellow), tiny pulse (red)

Multi-mode usage
-----

Hold one of the six buttons for 5 seconds to change mode. This setting is persisted when the module reboots. From left to right:

1. [Segment generator](#segment-generator)
2. [Advanced Segment generator](#advanced-segment-generator)
3. Segment generator with [slower free-running LFOs](#slower-free-running-lfos)
4. [Six DAHDSR envelope generators](#six-dahdsr-envelope-generators)
5. [Harmonic oscillator](#harmonic-oscillator), aka Ouroboros mode
6. Harmonic oscillator with [alternate controls](#harmonic-oscillator-with-alternate-controls)

For chained modules: adjacent, connected Stages will chain if both are in mode 1 (segment generator) or both are in mode 2 (adv. segment generator) or 3 (slow LFO).
Modes 4, 5, and 6 will split a chain.
So if you have connected modules in 1-1-2-3-6-2, 1-1 will be a chain, 2-3 will be a chain, and the last 2 will be isolated, like so: (1-1)-(2-3)-(6)-(2).
Changing a module's mode will automatically update the chain configuration (the LEDs will flash like on startup).
So, in the above example, if the module in mode 6 is changed to mode 2, the chaining configuration will become (1-1)-(2-3-2-2).
Modules with the original Stages firmware are ignored.

### Segment generator

This is the standard mode of the module, refer to the official [Stages manual][4]. This firmware is built on top of official [Stages 1.1][10] and [latest changes][11], therefore it includes **color-blind mode**, **S&H gate delay** and **LFO phase preservation**.

This fork adds the following features to this mode, none of which interfere with Stages normal operation; you can ignore any of them you don't care about and you won't even know they're there:

- **Polarity control for each segment type** (except non-looping ramp/green). Hold button and wiggle the pot to toggle polarity. Return the pot to its original position in order to preserve the shape/time that the segment had before you pressed the button. When bipolar, the segment will flash dull red about 1/sec.
    - Bipolar LFOs (single, looping, green) will output -5v to 5v rather than the normal 0v to 8v.
    - Bipolar step (orange) and hold (red) have an effective slider range of -8v to 8v rather than 0v to 8v.
- **Re-trigger control for ramp (green) segments**. Hold button and wiggle the pot toggle re-trigger behavior. When re-trigger is disabled, the segment will flash dull red about 1/sec.
    - With re-trigger disabled, ramp segments will cause rising gates to be ignored, whereas normally rising gates cause an envelope to re-trigger from the beginning
    - For instance, if you make an AD envelope, and disable re-trigger on the A, you get classic Maths/Serge function generator behavior, allowing for weird clock dividers, subharmonic generators, and so forth.
    - You can disable re-trigger on *any* ramp segment; the D could be disabled instead of the A, so if the envelope receives a trigger after the A (but before it ends), the trigger will be ignored.
- **Independent LFO (clocked and free) range control for each segment**:
    - For free-running, ranges are the same as Tides: 2 min to 2hz at the slowest, 0.125hz to 32.7hz (default and Stages' original range), and 8hz to about 2khz at the fastest. As with the original Stages and Tides, this range is further expandable by CV.
    - For clocked LFOs, clock multiplications are:
	- Slow: 1/32, 1/16, 1/8, 1/7, 1/6, 1/5, 1/4, 1/3, 1/2, 1
	- Medium: 1/4, 1/3, 1/2, 1, 2, 3, 4 (default; original Stages' behavior)
	- Fast: 1, 2, 3, 4, 5, 6, 7, 8, 12, 16
    - Hold the segment's button and move its slider to change LFO range. LFO range is indicated by the speed of the mode indicator LED's cycle. Note: artifacts appear at high frequencies depending on wave shape. Frequency has been capped at ~7khz.
- **Time range control for ramp segments**:
    - Hold button and move slider to bottom (fast: 1 ms to 2.2 sec), middle (default: 1 ms to 16 sec), or top (slow: 16 sec to 13 min).
    - Note: Unlike with LFOs, CV range is unchanged.
    - Range is indicated by a solid green light followed by a ramp down to black. Default is solid green.
- **Quantizing step and hold segments**. Hold button and move slider on step (yellow) and hold (red) segments to control quantization. The LED will flash and change color to indicate scale.
    - Scale, from bottom to top: unquantized (LED off), chromatic (red), major (yellow), pentatonic (red)
    - When quantized, the slider has an effective range of 0v-2v and is added *before* quantization, and thus transposes in key. It can thus be used as a quantized sequencer or to select [mode](https://en.wikipedia.org/wiki/Mode_\(music\)#Modern_modes)
	- Thus, minor (aeolian) is the 6th step in major
	- Minor pentatonic is the 4th step in pentatonic
    - The segments also have all their normal behavior: yellows can thus slew between notes,
    - *Note*: There is no visual indication of scale once the button is released. Sorry :( Open to ideas here, but the LEDs are already pretty overloaded.

This fork also improves some behavior of the original Stages.
While these change basic behavior, they primarily change it in places that Stages couldn't really be used before, so hopefully will feel like strict improvements.

- **Arbitrarily slow clocked LFOs**. Previously, clocked LFOs in Stages had a reset timeout at about 5 seconds; now, the reset timeout adapts to the clock cycle, allowing for arbitrarily slow clocked LFOs (logic taken from Marbles and Tides 2). The PLL tracking will now also reset when the segment type or frequency range changes.
- **Improved audio-rate clocked LFOs**. Previously, clocked LFOs used a PLL algorithm designed for low frequency clocks and rhythms. While this allows LFOs to adapt to more complex gate sequences, it introduces artifacts at audio rates. Clocked LFOs will now detect when they're receiving an audio rate signal and switch to an algorithm tuned to audio rates, allowing them to create harmonics of other voices. Logic adapted from Marbles and Tides 2.
- **Ramp segments track start and end value**. Previously, ramp segments would track end value but sample and hold start value. This gives you:
    - Built-in pseudo VCAs: For instance, in an ASR (green, looping red, green), plugging your signal into the red's CV in will appropriately attenuate it by attack and release.
    - Built-in pseudo crossfading: For instance, ramp-step-ramp-step (GYGY) with two signals going into the step segments will crossfade between the two signals every time it receives a gate. Ramp-hold-ramp-hold looping continuously crossfade.
- **Negative quantized values in extended sequencer**. The [official extended sequencer firmware](https://forum.mutable-instruments.net/t/stages-extended-sequencer-firmware/17493) supports semitone quantization for positive voltages. This firmware adds support for negative voltages, meaning that bipolar step segments give you two octaves of quantized values.
    - Note: using quantized steps in a quantized extended sequence can give unexpected results, as the extended sequence range adjustment and quantization is applied after the step's quantization.

For any settings that requires you hold the button to change, moving the slider or pot will disable loop mode changes or multi-mode changes (and disable the normal function of the pot), so you don't need to worry about holding the button for too long.
Also changes to those settings won't occur unless you move it's respective control; thus, you won't accidentally change the range on an LFO while changing it's polarity unless you move the slider.
If the slider is already in the position of the setting you want, simply wiggle it to one side and then back into the desired setting while the button is held.
After you release the button, you can then use the pot/slider as normal.
Finally, holding the button locks the value of the pot/slider so that the segment values won't change while you're changing properties (except as affected by those properties).
After releasing the button, the value of the pot/slider while move to the position within 2 seconds, allowing you to restore the original value without the changed position having too much of an effect.
This is indicated by the type LED/slider LED dimming and growing brighter as the value moves toward position.


[4]: https://mutable-instruments.net/modules/stages/manual/
[10]: https://mutable-instruments.net/modules/stages/firmware/
[11]: https://github.com/pichenettes/eurorack/commits/master

### Advanced Segment generator

This mode includes all the features of the [segment generator mode](#segment-generator), plus several additional features.
The features included in this mode change the standard workflow of Stages, such as the addition of a new segment type and the modification of the way existing segment types function; thus, they have been put it in a separate mode.

##### Random segment type
This mode adds a fourth segment type, **random segments**, which gives you access to several different types of random and chaotic CV.
This segment type comes after hold (red) segments in the cycle of segments and is indicated by an LED that continuously morphs between red and green.

When used as a single segment, random segments behave as follows:

- Ungated, non-looping: A uniform random CV generator. Just your basic random output, with controllable frequency.
    - Slider/CV: Frequency in V/oct.
    - Button + slider: Frequency range, same as LFOs. Bottom/slow is 2 min to 2 hz. Middle/default is ~8 secs to ~32hz. Top/fast is ~8hz to ~2khz.
    - Pot: Portamento
    - Button + pot: Polarity. Unipolar ranges between 0v and 8v. Bipolar between -5v and 5v.
- Ungated, looping: A smoothly chaotic system, Chen's [double-scroll attractor](https://en.wikipedia.org/wiki/Multiscroll_attractor), a variant of Chua's attractor, with `a=42`, `b` between 1 and 6, and `c=28`.
    - Slider/CV: Frequency roughly in V/oct (the rate at which the simulation updates would give V/oct if the system was periodic... which it isn't...)
    - Button + slider: Frequency range. Rough periods (defined by crossing midpoint twice): Bottom/slow is ~33 mins (!) to about 8 secs, with even slower available with negative CV. Middle/default is 2 mins to 2hz. Top/slow is 8 secs to ~32hz.
    - Pot: Shape. Specifically, controls `b`, with CCW=1 and CW=6.
        - All the way CCW will give a curve that hovers around 4v, and spikes up or down at regular intervals (though it can get stuck around a periodic attractor with certain initial conditions; wiggle pot to free it).
        - 12:00 will give a nice variety of shapes and wiggles around a high point and low point, with some macro motion as well.
        - CW will give more jagged, hopping between high and low points that it wiggles, but generally more extreme behavior.
    - Button + pot: Polarity. Unipolar ranges between 0v and 8v. Bipolar -5v and 5v. Exact range depends on pot position.
    - You can play around with a [simulation of the attractor](https://netlogoweb.org/launch#https://gist.githubusercontent.com/qiemem/e36e443c8808a5a1d4e3bb33ed6900d3/raw/bc53e3c9a3193145a1923f69027f706730d03cd3/chaos.nlogo) that I made to tune it. The x-axis in the plots should correspond very closely with time in Stages. Behavior will differ somewhat due to floating point representation.
- Gated, non-looping: An implementation of Tom Whitwell's [Turing Machine](https://www.modulargrid.net/e/music-thing-modular-turing-machine-mk-ii--). The segment contains a 16 bit shift register. Output is the current value of the shift register (unquantized). On a rising gate, the register rotates, and the bit at the end is copied to the beginning with a probability of flipping. A great algorithm for controllable randomness, as you can lock loops you like, or let them slowly evolve.
    - Slider/CV: Probability of flipping the copied bit. At 0, the sequence will be locked. At 1, the copied bit will always flip, allowing for locked sequences of twice the length. At 0.5, the copied bit will be completely random.
    - Pot: Number of steps, from 1 to 16.
    - Button + pot: Polarity. Unipolar ranges between 0v and 8v. Bipolar between -5v and 5v.
- Gated, looping: A chaotic sequence generator using the [logistic map](https://en.wikipedia.org/wiki/Logistic_map). The logistic map is a chaotic system inspired by population dynamics that can range from small repeating sequences to ever evolving chaos. A rising gate applies a single iteration of the logistic map. Thanks to the [XAOC Batumi alternate firmware](https://github.com/xaocdevices/batumi/tree/alternate) for the idea (though the implementations are slightly different)!
    - Slider/CV: The reproduction rate (3.5 to 4). At the lowest, gives a simple sequence with period of 4. As it increases, the period keeps growing until it diverges. The character of the generated sequences varies greatly throughout the range.
    - Pot: Portamento
    - Button + pot: Polarity. Unipolar ranges between 0v and 8v. Bipolar between -5v and 5v.

When used in an envelope, a **non-looping** random segment will act like a step (orange) segment with a Turing Machine as input.
When the envelope reaches the random segment, the envelope will output the value of the segment's shift register and hold until the envelope receives another trigger.
When the envelope receives the trigger, the envelope will move on from the random segment, and the segment will advance its shift register as described above.

A **looping** random segment will act like a looping hold (red) segment with a Turing Machine as input.
When the envelope reaches the random segment, the envelope will output the value of segment's shift register until the envelope's gate goes low, at which point the segment's shift register will advance.

Tip: Don't have or want to use a quantiser, but want random melodies?
Set one segment to an audio-rate "LFO" (green, looping, ungated: hold button and move slider to the top to set slider to audio rate, and then move it to desired frequency).
Then, clock another bipolar (hold button, wiggle pot) LFO segment with the first.
Finally, feed a random segment with one of the above algorithms into the CV of the clocked, bipolar segment.
That segment will then quantise to harmonies of the first segment.

##### Single, non-gated, non-looping ramp (green) segments slew with independent rise and fall

In normal Stages, single, non-gated, non-looping ramp segments didn't do anything.
In advanced mode, they act as slew segments with independent rise and fall control.
Slider controls fall time (just like in single, gated, non-looping ramp segments) and pot controls rise time.
CV input gives the target value.
They have the same natural RC curve of slewed yellow segments (concave rise and convex fall), making them natural for use as audio envelopes when paired with a linear VCA.
These segments have a many uses beyond standard slew:

- ASR envelopes: Patch your gate into the *CV* input (not gate). When the gate is high, the segment will rise and hold until the gate is release, at which point the segment will fall. This can be a nice alternative to decay segments (single, gated, non-looping green) when you want to avoid a clicky attack, or can be used as a general purpose ASR.
- Envelope follower: Patch an audio signal into CV. Set rise short and fall longer. The segment will follow the amplitude of the audio signal. Adjust fall time to taste. Flip rise and fall to create an inverted envelope follower.

Pot+slider changes polarity. In unipolar mode (default), a full-wave rectifier is applied to the incoming signal, improving behavior as an envelope follower. In bipolar mode, no rectifier is applied.

##### Single, looping step (yellow) segments attenuate

Single, looping step (yellow) segments attenuate instead of slew (both gated and non-gated).
In normal Stages, these have identical behavior to non-looping.
Pot controls attenuation amount (0 to 1 in unipolar; -1 to 1 in bipolar).
Non-gated segments will continuously attenuate, giving you Shades-like behavior.
Gated segments will still sample and hold, but with attenuation instead slew.

- Slider/CV: Offset applied to output
- Pot: Attenuation amount; attenuverts when segment is bipolar

These segments can be very handy to control the range of LFOs and random segments.
Turning on quantization with these segments can also be quite handy.
Feeding in an LFO gives you arpeggios with controllable range.
Feeding in a TM segment gives a generative melodies.

##### Single, gated, looping hold (red) segments have probability control

The pot on single, gated, looping hold segments now controls their probability of firing.
Previously, the pot did not do anything.
Fully CW will cause it to always fire (the original behavior) and fully CCW will cause it to never fire.

### Slower free-running LFOs

In this mode, Stages behaves exactly like the standard segment generator mode, except free-running LFOs (i.e. single green looping segments) are [eight time slower][5].
This mode contains all the new features of [segment generator mode](#segment-generator).
This fork applies this 8x slowdown to each of the LFO ranges, so while the default is the same as in joeSeggiola's original, much slower LFOs may be achieved (16 minutes), while also mixing with faster LFOs.

Note: Since LFO range configuration has been integrated in as a segment property, this mode may be removed to make space for other things.

[5]: https://forum.mutable-instruments.net/t/stages/13643/54


### Six DAHDSR envelope generators

The module transforms into a generator of six identical envelopes. **Sliders** controls the duration (or level) of each stage of all envelopes. From left to right:

1. Duration of the **delay** phase
2. Duration of the **attack** phase
3. Duration of the **hold** phase
4. Duration of the **decay** phase
5. Level of the **sustain** phase
6. Duration of the **release** phase

Each duration goes from 0 to 10 seconds. Each value can be **modulated** using **TIME/LEVEL** inputs. The hold phase is always at maximum level (8V). Each stage can be "disabled" by setting the slider to the bottom; the LED on the slider will turn off to indicate that. For example, set sliders 1 and 3 to zero to get six standard ADSR envelopes.

**SHAPE/TIME** pots 2, 4 and 6 control the **shape** of the corresponding ramp stages, from accelerating through linear, to decelerating. Pots 1, 3 and 5 are unused.

**GATE** inputs are used to activate each of the six envelopes, which can be taken from the corresponding outputs on the bottom of the module. LEDs below pots show the current phase of each envelope: green for delay/attack/hold/decay, orange for sustain, red for release, off when idle. Pressing a **button** will trigger the corresponding envelope manually, like it's a gate signal.


### Harmonic oscillator

This mode was normally accessible on the non-modified firmware by [chaining][1] the module with itself (hence the name "Ouroboros" mode). This firmware simply adds the ability to switch at runtime, without using the cable on the back of the module. All credits goes to [Stages author][9], obviously.

[9]: https://github.com/pichenettes

This fork further adds frequency range controls to this mode, allowing it to be used as a set of interrelated LFOs.

The left-most column of the module acts a little different from the others:

- Slider is for **coarse tuning** of the main oscillator
- Pot is for **fine tuning**
- CV is a **1V/oct** input for the root pitch
- Output is an **audio mix** of all the harmonics

Each one of the next five columns controls a partial:

- Pot selects the **harmonic ratio** in relation to the root pitch
- Slider and CV input control its **volume** in the mix
- Gate is for **strumming**, i.e. temporarily increase the volume with a fast decay envelope

The **buttons** cycle through different waveform for each harmonic, including the root one. Selected waveform is shown using LEDs colors:

- Green: **sine**
- Orange: **triangle**
- Red: **square**
- Flashing green (long-press): **sawtooth**
- Flashing orange: square with **small pulse width**
- Flashing red: square with **smaller pulse width**

To change the frequency range of this mode, hold the leftmost button and move the leftmost slider to the top, middle, or bottom (same as adjusting LFO range in segment generator mode).
The ranges are as follows:

- Top: 16hz to ~4khz (C0 to C8); the default range
- Middle: 0.125hz to 32hz
- Bottom: 2 min to 2hz

The middle and bottom ranges are considered "LFO" ranges.
When operating in an LFO range, this mode has a few adjusted behaviors.

- Volume control now affects the amplitude of the segments individual output in addition to its amplitude in the mix output. In audio range, volume only affects amplitude in the mix output. This gives you the ability to attenuate your LFOs.
- A rising gate on a segment's gate input will reset the phase of the LFO. It will still "strum" the LFO in the mix output (but not the individual output).

Tip: To remove an LFO from the mix, but continue to use its individual output, plug a dummy cable into its gate. This is especially useful for removing the "root" LFO from the mix (by plugging a cable into the leftmost gate input).

Note that, as in the original Stages' harmonic oscillator mode, it is possible to set frequencies in between two harmonics.
This can be really handy for creating LFOs with interesting phasing, but may be confusing at first if you're expecting the LFOs to stay exactly in sync.
The frequencies will "snap" to defined harmonics, so it shouldn't be too hard to get things to stay in time.

The harmonics, from high to low are 8x, 6x, 5x, 4x, 3x, 2x, 1.5x, 1/2x, 1/4x.

### Harmonic oscillator with alternate controls

Same as harmonic oscillator, but controls for each partial (columns 2 to 6) are swapped:

- Pot controls the **volume** in the mix
- Slider and CV input sets the **harmonic ratio** in relation to the root pitch

This way it is possible to modulate (and therefore sequence) the harmonics with external CV.

The frequency range in this mode can be controlled just as with the normal harmonic oscillator mode (hold leftmost button and move leftmost slider).

Known issues
------------

- **Having too many clocked LFOs can create tracking problems.** This firmware is somewhat more computationally intensive than the stock. As a result, clocked LFOs (which are particularly computationally intensively) can have difficulty tracking if there are too many. Currently, five clocked LFOs can be comfortably run simultaneously. Adding a sixth will give you some... interesting results.

See https://github.com/qiemem/eurorack/issues for full list of issues.

Changelog
---------

Fork:

- [v1.1.0](https://github.com/qiemem/eurorack/releases/tag/v1.1.0)
    - Merge second version of extended sequencer mode from latest official firmware.
    - Add support for chained modules! See #13, #14, and #18. Thanks to [pyerbass](https://forum.mutable-instruments.net/t/stages-qiemems-alternative-firmware/17506/7) and others for being brave enough to test for me, to 0netwo0netwo for selling me their Stages, and to Samet for lending me their Stages!
    - Add support for turing machine segments in the extended sequencer. See #20.
    - Make the pot on single gated looping red segments control probability of firing. Suggested by [pyerbass](https://forum.mutable-instruments.net/t/stages-qiemems-alternative-firmware/17506/23).
    - Make single ungated looping random segments produce a smooth chaotic attractor. See #24. Suggested by [pyerbass](https://forum.mutable-instruments.net/t/stages-qiemems-alternative-firmware/17506/23).
    - Add slider range control for ramp segments. See #26 and #30. Implemented by @w-winter.
    - Add full-wave rectification for unipolar rise and fall segments (single, non-gated, non-looping green) to improve use as envelope followers. See #27 and #32. Implemented by @w-winter.
    - Significant optimization to the clocked LFO and UI code, allowing five clocked LFOs to be run simultaneously. See #9.
    - Minor fix when using multiple quantized segments. See #19.
    - UI improvement to rise and fall segments. See #17 (though done in a different way than described).
- [v1.0.2](https://github.com/qiemem/eurorack/releases/tag/v1.0.2)
    - Fixed groups of segments splitting correctly when new gates are patched. See #12. Thanks to pyerbass on the MI forum for catching this!
- [v1.0.1](https://github.com/qiemem/eurorack/releases/tag/v1.0.1)
    - Merge in extended sequencer from latest official firmware!
    - Ensure that 16 steps is reachable with the pot in TM mode. See #10.
    - Add support for voltage below 0v and above 8v to alt. harmonic oscillator mode. Previously the given segment would just output a constant voltage.
    - Significantly improve performance enabling more simultaneous clocked LFOs. See #9. Work in progress.
- [v1.0.0](https://github.com/qiemem/eurorack/releases/tag/v1.0.0)
    - Add harmonic oscillator frequency range control. Suggested by MW user [thetechnobear](https://www.muffwiggler.com/forum/viewtopic.php?f=16&t=235787&p=3332097#p3326831).
    - Add optional quantization to step and hold segments. Suggested by @jb0000. See #1.
    - Make single looping step segments attenuate (removes T&H). Suggested by MW users [Carrousel](https://www.muffwiggler.com/forum/viewtopic.php?p=3334476&sid=2f3e61660b8023782673d8e076106be3#p3334476) and [SavageMessiah](https://www.muffwiggler.com/forum/viewtopic.php?p=3334492&sid=2f3e61660b8023782673d8e076106be3#p3334492).
    - Make single ngnl ramp segments slew with independent rise and fall. Implementation suggested by @jb0000. See #4.
    - Improve clocked LFO audio rate and reset behavior
    - Improve audio rate cap
    - Make ramp segments track start value
    - Lock pot/slider values when holding button and then be gradually restored. Suggested by @jb0000. See #6.
    - Add cheat sheets to README. Having a quick reference suggested by MW user [baleen](https://www.muffwiggler.com/forum/viewtopic.php?f=16&t=235787&start=175#p3311760).
    - Expand range of fast and slow clocked LFOs for with musically useful values (1/16 and 1/32 for slow; 12 and 16 for fast).
    - Fix random segments crashing the module when switching to normal segment generator mode
    - Fix Stages' LFO range messing up harmonic oscillator control.

- [v1.0.0-beta4](https://github.com/qiemem/eurorack/releases/tag/v1.0.0-beta4)
    - Add advanced mode with random segments (uniform random, Turing Machine, and logistic map) and T&H (T&H moved from segment generator mode)
    - Change polarity/re-trigger control to toggle so that you can change polarity/re-trigger behavior without affecting shape/time. [Suggested by MW user jube.](https://www.muffwiggler.com/forum/viewtopic.php?f=16&t=198455&start=975#p3317120)
    - Fix an issue where LFO range selection LED brightness would override Ouroboros LED brightness. [Reported by MW user gelabs.](https://www.muffwiggler.com/forum/viewtopic.php?f=16&t=198455&start=975#p3316749)
    - Fix an issue where segments set to bipolar step or hold in normal mode would could contribute negative CV in other modes
- [v1.0.0-beta3](https://github.com/qiemem/eurorack/releases/tag/v1.0.0-beta3)
    - Fix LFO range resetting when the loop status of a different segment changed. [Reported by MW user gelabs](https://www.muffwiggler.com/forum/viewtopic.php?f=16&t=198455&start=950#p3314104).

- [v1.0.0-beta2](https://github.com/qiemem/eurorack/releases/tag/v1.0.0-beta2)
    - New hold-button-move-pot control scheme
    - Add LFO frequency range control
    - Allow for arbitrarily slow clocked LFOs
    - Add track & hold support
    - Signify bipolarity with dim red blink 1/sec
    - Revert slow LFO mode to joeSeggiola's original (works with LFO frequency range selection)

- [v1.0.0-beta](https://github.com/qiemem/eurorack/releases/tag/v1.0.0-beta)
    - Add bipolar mode for each segment type
    - Add re-trigger control for ramps
    - Change slow LFO mode to expand slider range

Original:

- [**v3**][103]: Fixed multi-mode switch to avoid unwanted segment loop toggle, independent permanent storage for segments configuration and harmonic oscillator waveform selection.
- [**v2**][102]: Alternate controls mode for harmonic oscillator.
- [**v1**][101]: Initial release, merging slower LFOs and Ouroboros toggle into a single firmware, together with a new 6xDAHDSR mode.

[101]: https://github.com/joeSeggiola/eurorack/releases/tag/stages-multi-v1
[102]: https://github.com/joeSeggiola/eurorack/releases/tag/stages-multi-v2
[103]: https://github.com/joeSeggiola/eurorack/releases/tag/stages-multi-v3


Feedback
--------

Please [let me know][6] if you encounter [issues][7] with my firmware modifications, or if you have ideas for additional modes.

[6]: https://github.com/qiemem/eurorack/issues/new
[7]: https://github.com/qiemem/eurorack/issues

