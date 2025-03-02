Mutable Instruments Stages multi-mode firmware
==============================================

This is an unofficial firmware for Mutable Instruments Stages. It was originally created by joeSeggiola and started as a modification to let you enter and exit the "Ouroboros" mode (the **harmonic oscillator** easter egg) at runtime, while the module is powered on. Then, it evolved to add the ability to modulate harmonic ratios and provide completely alternative modes that transform the module into **six DAHDSR envelope generators**.

This fork further adds the following to segment generator mode:

- **Bipolar LFOs, steps, and holds**: Toggle a segment's polarity by holding its button and wiggling its pot
- **Very-slow and audio-rate LFOs**: Change an LFO's frequency range by holding its button and moving its slider to the bottom for slow LFOs, to the middle for the default speed, near the top for audio-rate with the normal variable shape LFOs, and to the very top for the dedicated audio-rate oscillators added in the final v1.2 release of the [official firmware](https://pichenettes.github.io/mutable-instruments-documentation/modules/stages/firmware/)
- **Re-trigger control**: hold a ramp segment's button and wiggle its pot to toggle re-trigger behavior; this allows for DUSG/Maths-style clock dividers, subharmonic generators, and more!
- **Fast and slow envelopes**: Change the range of a ramp's slider by holding its button and moving the slider to the top, middle, or bottom
- **Arbitrarily slow-clocked LFOs and improved audio-rate clocked LFOs** [^made-official]
- **Resettable LFOs**: hold the segment's button while you patch the gate input to make the LFO reset on triggers instead of clocking
- **Hold and step segment quantization**: Hold the segment's button and move its slider to select scale; the slider transposes within the key, allowing selection of mode
- **Start and end value tracking for ramp segments**: This can be used as a kind of VCA or crossfader [^made-official]
- **Probabilistic gates**: A single, looping, gated red segment's pot now controls its probability of firing, giving you a mini-branches (the pot used to do nothing in this case) [^made-official]

[^made-official]: This behavior was incorporated into the official firmware in [v1.2](https://pichenettes.github.io/mutable-instruments-documentation/modules/stages/firmware/).

See the segment generator mode's [usage instructions](#segment-generator) for details. None of these features interfere with the normal workflow of Stages.

This fork also adds a new mode, [advanced segment generator](#advanced-segment-generator), which adds

- **Random segment type**: A fourth type of segment, which gives access to four different random/chaotic algorithms:
    - Double-scroll attractor, a smooth chaotic system similar to the Lorenz system (single, non-looping, non-gated)
    - Probabilistic digital shift register (ie emulation of Tom Whitwell's Turing Machine) (non-looping, gated)
    - Random LFO, with variable shapes from stepped to smooth to smooth random walk and adjustable frequency range, including audio-rate noise (single, looping, non-gated)
    - Clocked random LFO (single, looping, gated)
- **Slew with independent rise and fall times**: Single, non-gated, non-looping ramp (green) segments slew with independent rise and fall: CV is target value, pot is rise, and slider is fall; this can be used as an AR envelope or an envelope follower as well
    - Unipolar and bipolar mode: applies full-wave rectification to incoming signal in unipolar mode for optimal use as an envelope follower
- **Attenuverter segments**: Single, looping step (yellow) segments attenuate instead of slew. Non-looping still slew, so no functionality is lost.

Finally, this fork allows you to control the frequency range of the harmonic oscillator mode (aka ouroboros mode; the Stages easter egg), giving access to 6 harmonically related LFOs, as well as the range of harmonics of the individual channels.

This fork includes the changes in official firmware 1.2 as well as [the extended sequencer official firmware](https://forum.mutable-instruments.net/t/stages-extended-sequencer-firmware/17493).

This firmware should fully support [chained modules](https://mutable-instruments.net/modules/stages/manual/#chaining-modules)! See [multi-mode usage](#multi-mode-usage).


Check out this great video overview of the fork as well as of the extended sequencer by Peakae, though it's a bit out of date at this point:

[![](https://img.youtube.com/vi/_-rgoM_oNE0/0.jpg)](https://www.youtube.com/watch?v=_-rgoM_oNE0)

Download and installation
-------------------------

📦 Download the **[latest WAV file here][2]** and follow the [firmware update procedure here][3]. Source code is available [here][8].
joeSeggiola's original source code is available [here][9].
stages-bipolar-vX.X.X.wav contains the normal version of the firmware while stages-bipolar-flipped-vX.X.X.wav contains the flipped version for using Stages upside-down.
Note that you press the same button to enter firmware-update mode regardless of using the normal or flipped version: the rightmost button when right-side-up / the leftmost button when upside-down.

IMPORTANT: Installation will clear the module settings if coming from a different firmware. Right after updating from an earlier version of this fork, the stock Stages firmware or joeSeggiola's version, Stages may continuously cycle between green, orange, and red LEDs. Turning the module off and on again should restore functionality. This happens because this fork expands the amount of data stored for each segment, so will be incompatible with the settings stored from a different firmware. If you encounter problems, please let me know, either in a GitHub issue or otherwise.

[2]: https://github.com/qiemem/eurorack/releases/latest
[3]: https://pichenettes.github.io/mutable-instruments-documentation/modules/stages/manual/#firmware
[8]: https://github.com/qiemem/eurorack/tree/stages-multi/stages
[9]: https://github.com/joeSeggiola/eurorack/tree/stages-multi/stages

Cheatsheet
---

Huge thank you to Philip Belesky for making this phenomenal, comprehensive [cheatsheet](https://raw.githubusercontent.com/qiemem/eurorack/bipolar/stages/resources/Stages%20Qiemem%20Cheatsheets.pdf).
It provides a visual guide to all button combinations in each of the modes, how values are laid out along each pot, and even a list of common configurations for segment groups.
I highly recommend checking it out!

Reference tables
---


- Button + slider: Hold button and move slider to top, middle, or bottom to change value
- Button + pot: Hold button and wiggle pot to toggle

Control overview (modes in order of corresponding button; hold button for 5 seconds to switch):
| Mode             | Button   | Hold button 1s | Slider + CV                                                    | Pot                                                                       | Button + slider                                                  | Button + pot                                            |
| ---------------- | -------- | -------------- | -------------------------------------------------------------- | ------------------------------------------------------------------------- | ---------------------------------------------------------------- | ------------------------------------------------------- |
| Segment gen      | Seg type | Toggle ➰     | <pre>Time:  🟢<br>Level: 🟡🔴</pre>                         | <pre>Shape: 🟢🟡<br>Time:  🔴                                          | <pre>Time range:  🟢<br>Quant scale: 🟡🔴</pre>               | <pre>Polarity: ➰🟢,🟡🔴<br>Re-trig:  🟢</pre>     |
| Adv segment gen  | Seg type | Toggle ➰     | <pre>Time:  🟢☢️️<br>Level: 🟡🔴<br>Prob:  🕟☢️</pre> | <pre>Shape: 🟢🟡☢️️<br>Atten: ➰🟡<br>Time:  🔴<br>Steps: 🕟☢️ | <pre>Time range:  🟢☢️️<br>Quant scale: 🟡🔴,🕟☢️</pre> | <pre>Polarity: ➰🟢,🟡🔴☢️<br>Re-trig:  🟢</pre> |
| Slow LFO         | Seg type | Toggle ➰     | <pre>Time:  🟢<br>Level: 🟡🔴</pre>                         | <pre>Shape: 🟢🟡<br>Time:  🔴                                          | <pre>Time range:  🟢<br>Quant scale: 🟡🔴</pre>               | <pre>Polarity: ➰🟢,🟡🔴<br>Re-trig:  🟢</pre>     |
| DAHDSR           | Gate     | -              | Time / level (5)                                               | Shape (246)                                                               | -                                                                | -                                                       |
| Harmonic osc     | Shape    | Toggle shapes  | Freq (1) / amp                                                 | Tune (1) / harmony                                                        | Freq range (1)                                                   | -                                                       |
| Alt harmonic osc | Shape    | Toggle shapes  | Freq (1) / harmony                                             | Tune (1) / amp                                                            | Freq range (1)                                                   | -                                                       |

- 🟢=ramp, 🟡=step, 🔴=hold, ☢️=random, 🗘=looping, 🕟=clocked/gated
- 🕟☢️ refers only to non-looping gated random segments in the above table (i.e. digital shift registers); ☢️ refers to all other random segments.

Single segment types in segment generator modes (* = advanced mode; all other behaviors are unchanged from original Stages):
| Segment type          | Behavior           | Slider + CV      | Pot         | Button + slider | Button + pot        |
| --------------------- | ------------------ | ---------------- | ----------- | --------------- | ------------------- |
| Green                 | Zero / R&F slew*   | Fall time*       | Rise time*  | Time range*     | Rectification*      |
| Green looping         | LFO                | Freq             | Shape       | Freq range      | Polarity            |
| Green gated           | Decay              | Time             | Shape       | Time range      | Re-trigger behavior |
| Green gated, looping  | Clocked LFO        | Div/mul          | Shape       | Div/mul range   | Polarity            |
| Yellow                | Slew               | Offset           | Time        | Quant scale     | Slider polarity     |
| Yellow looping        | Slew / Atten*      | Offset           | Time / Att* | Quant scale     | Slider polarity     |
| Yellow gated          | S&H                | Offset           | Slew        | Quant scale     | Slider polarity     |
| Yellow gated, looping | S&H / Atten*       | Offset           | Time / Att* | Quant scale     | Slider polarity     |
| Red                   | Delay              | Offset           | Time        | Quant scale     | Slider polarity     |
| Red looping           | Delay              | Offset           | Time        | Quant scale     | Slider polarity     |
| Red gated             | Timed pulse        | Offset           | Pulse width | Quant scale     | Slider polarity     |
| Red gated, looping    | Gate generator     | Offset           | Probability | Quant scale     | Slider polarity     |
| GR*                   | Double scroll      | Freq             | Slew        | Freq range      | Polarity            |
| GR looping*           | Random LFO         | Freq             | Smoothness  | Freq range      | Polarity            |
| GR gated*             | Dig shift reg      | Prob of bit flip | Steps       | Quant scale     | Polarity            |
| GR gated, looping*    | Clocked random LFO | Div/mul          | Smoothness  | Div/mul range   | Polarity            |

Segment types in groups (only green~red and button+pot are different from original Stages):
| Segment type | Behavior       | Slider + CV      | Pot   | Button + slider | Button + pot        |
| ------------ | -------------- | ---------------- | ----- | --------------- | ------------------- |
| Green        | Ramp           | Time             | Shape | Time range*     | Re-trigger behavior |
| Yellow       | Step           | Offset           | Slew  | Quant scale     | Slider polarity     |
| Red          | Hold           | Offset           | Time  | Quant scale     | Slider polarity     |
| Red looping  | Sustain        | Offset           | -     | Quant scale     | Slider polarity     |
| Green~red*   | Turing machine | Prob of bit flip | Steps | Quant scale     | Polarity            |

Segment generator LED codes:
- Blink dim red 1/sec: Bipolar / no re-trigger (for ramp) / no rectification (for green rise & fall slew)
- Oscillations between color and black: Looping mode
- Speed of looping mode oscillations (for LFO and random LFO segments): LFO freq range
- Solid green followed by ramp down: Ramp time range (default is solid)
- Smooth oscillations between green, yellow, and red: Random segment type in advanced mode
- Fading in of slider/type LED after button + slider/pot: Value of slider/pot adjusting to position (giving you a chance to change a property without affecting slider/pot value much)
- Rapid flipping between green and red: Audio-rate oscillator with alternative waveforms


Harmonic oscillator shapes:
- Normal: Sine (green), triangle (yellow), square (red)
- Looping: Ramp (green), small pulse (yellow), tiny pulse (red)

Multi-mode usage
-----

Hold one of the six buttons for 5 seconds to change mode. This setting is persisted when the module reboots. From left to right:

1. [Segment generator](#segment-generator)
2. [Advanced Segment generator](#advanced-segment-generator)
3. [Six independent DAHDSR envelope generators](#six-independent-dahdsr-envelope-generators)
4. [Six identical DAHDSR envelope generators](#six-identical-dahdsr-envelope-generators)
5. [Harmonic oscillator](#harmonic-oscillator), aka Ouroboros mode
6. Harmonic oscillator with [alternate controls](#harmonic-oscillator-with-alternate-controls)

For chained modules: adjacent, connected Stages will chain if both are in mode 1 (segment generator) or both are in mode 2 (adv. segment generator).
Modes 3, 4, 5, and 6 will split a chain.
So if you have connected modules in 1-1-2-2-6-2, 1-1 will be a chain, 2-2 will be a chain, and the last 2 will be isolated, like so: (1-1)-(2-2)-(6)-(2).
Changing a module's mode will automatically update the chain configuration (the LEDs will flash like on startup).
So, in the above example, if the module in mode 6 is changed to mode 2, the chaining configuration will become (1-1)-(2-2-2-2).
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
    - Hold the segment's button and move its slider to change LFO range. LFO range is indicated by the speed of the mode indicator LED's cycle. Frequency has been capped at ~7khz.
    - Holding the button and moving the slider to the very top will activate the audio-rate oscillator added to the official firmware in v1.2. These oscillators are fully bandlimited and have more diverse set of timbers.
    - Holding the button while you patch the gate input will cause the LFO to reset on triggers instead of clocking.
- **Time range control for ramp segments**:
    - Hold button and move slider to bottom (fast: 1 ms to 2.2 sec), middle (default: 1 ms to 16 sec), or top (slow: 16 sec to 13 min).
    - Note: Unlike with LFOs, CV range is unchanged.
    - Range is indicated by a solid green light followed by a ramp down to black. Default is solid green.
- **Quantizing step and hold segments**. Hold button and move slider on step (yellow) and hold (red) segments to control quantization. The LED will flash and change color to indicate scale.
    - Scale, from bottom to top: unquantized (LED off), chromatic (red), major (yellow), pentatonic (green)
    - When quantized, the slider has an effective range of 0v-2v and is added *before* quantization, and thus transposes in key. It can thus be used as a quantized sequencer or to select [mode](https://en.wikipedia.org/wiki/Mode_\(music\)#Modern_modes)
	- Thus, minor (aeolian) is the 6th step in major
	- Minor pentatonic is the 4th step in pentatonic
    - The segments also have all their normal behavior: yellows can thus slew between notes,
    - *Note*: There is no visual indication of scale once the button is released. Sorry :( Open to ideas here, but the LEDs are already pretty overloaded.

This fork also improves some behavior of the original Stages.
While these change basic behavior, they primarily change it in places that Stages couldn't really be used before, so hopefully will feel like strict improvements.

- **Arbitrarily slow clocked LFOs**. Previously, clocked LFOs in Stages had a reset timeout at about 5 seconds; now, the reset timeout adapts to the clock cycle, allowing for arbitrarily slow clocked LFOs (logic taken from Marbles and Tides 2). The PLL tracking will now also reset when the segment type or frequency range changes. *Note*: This change was incorporated into the official firmware in v1.2.
- **Improved audio-rate clocked LFOs**. Previously, clocked LFOs used a sophisticated clock prediction algorithm designed for low-frequency clocks and rhythms. While this allows LFOs to adapt to more complex gate sequences, it introduces artifacts at audio rates. Clocked LFOs will now detect when they're receiving an audio rate (32.7hz, aka C1, for more than 250ms) signal and switch to an algorithm tuned to audio rates, allowing them to create harmonics of other voices.
- **Bandlimiting of audio-rate LFOs**. LFOs running above 32.7hz will use bandlimiting to prevent aliasing. This results in much better-sounding square and saw waves.
- **Ramp segments track start and end value**. Previously, ramp segments would track end value but sample and hold start value. This gives you:
    - Built-in pseudo VCAs: For instance, in an ASR (green, looping red, green), plugging your signal into the red's CV in will appropriately attenuate it by attack and release.
    - Built-in pseudo crossfading: For instance, ramp-step-ramp-step (GYGY) with two signals going into the step segments will crossfade between the two signals every time it receives a gate. Ramp-hold-ramp-hold looping continuously crossfade.
    - *Note*: This change was incorporated into the official firmware in v1.2.
- **Negative quantized values in extended sequencer**. The [official extended sequencer firmware](https://forum.mutable-instruments.net/t/stages-extended-sequencer-firmware/17493) supports semitone quantization for positive voltages. This firmware adds support for negative voltages, meaning that bipolar step segments give you two octaves of quantized values.
    - Note: using quantized steps in a quantized extended sequence can give unexpected results, as the extended sequence range adjustment and quantization is applied after the step's quantization.

For any settings that require you to hold the button to change, moving the slider or pot will disable loop mode changes or multi-mode changes (and disable the normal function of the pot), so you don't need to worry about holding the button for too long.
Also, changes to those settings won't occur unless you move its respective control; thus, you won't accidentally change the range on an LFO while changing its polarity unless you move the slider.
If the slider is already in the position of the setting you want, simply wiggle it to one side and then back into the desired setting while the button is held.
After you release the button, you can then use the pot/slider as normal.
Finally, holding the button locks the value of the pot/slider so that the segment values won't change while you're changing properties (except as affected by those properties).
After releasing the button, the value of the pot/slider will move to the position within 2 seconds, allowing you to restore the original value without the changed position having too much of an effect.
This is indicated by the type LED/slider LED dimming and growing brighter as the value moves toward position.


[4]: https://mutable-instruments.net/modules/stages/manual/
[10]: https://mutable-instruments.net/modules/stages/firmware/
[11]: https://github.com/pichenettes/eurorack/commits/master

### Advanced Segment generator

This mode includes all the features of the [segment generator mode](#segment-generator), plus several additional features.
The features included in this mode change the standard workflow of Stages, such as the addition of a new segment type and the modification of the way existing segment types function; thus, they have been put in a separate mode.

##### Random segment type
This mode adds a fourth segment type, **random segments**, which gives you access to several different types of random and chaotic CV.
This segment type comes after hold (red) segments in the cycle of segments and is indicated by an LED that continuously morphs between red and green.

When used as a single segment, random segments behave as follows:

- Ungated, non-looping: A smoothly chaotic system, Chen's [double-scroll attractor](https://en.wikipedia.org/wiki/Multiscroll_attractor), a variant of Chua's attractor, with `a=42`, `b` between 1 and 6, and `c=28`.
    - Slider/CV: Frequency roughly in V/oct (the rate at which the simulation updates would give V/oct if the system was periodic... which it isn't...)
    - Button + slider: Frequency range. Rough periods (defined by crossing midpoint twice): Bottom/slow is ~33 mins (!) to about 8 secs, with even slower available with negative CV. Middle/default is 2 mins to 2hz. Top/fast is 8 secs to ~32hz.
    - Pot: Shape. Specifically, controls `b`, with CCW=1 and CW=6.
        - All the way CCW will give a curve that hovers around 4v, and spikes up or down at regular intervals (though it can get stuck around a periodic attractor with certain initial conditions; wiggle pot to free it).
        - 12:00 will give a nice variety of shapes and wiggles around a high point and low point, with some macro motion as well.
        - CW will give more jagged, hopping between high and low points that it wiggles, but generally more extreme behavior.
    - Button + pot: Polarity. Unipolar ranges between 0v and 8v. Bipolar -5v and 5v. Exact range depends on pot position.
    - You can play around with a [simulation of the attractor](https://netlogoweb.org/launch#https://gist.githubusercontent.com/qiemem/e36e443c8808a5a1d4e3bb33ed6900d3/raw/bc53e3c9a3193145a1923f69027f706730d03cd3/chaos.nlogo) that I made to tune it. The x-axis in the plots should correspond very closely with time in Stages. Behavior will differ somewhat due to floating point representation.
- Gated, non-looping: A probabilistic digital shift register, based on Tom Whitwell's [Turing Machine](https://www.modulargrid.net/e/music-thing-modular-turing-machine-mk-ii--). The segment contains a 16 bit shift register. Output is the current value of the shift register. On a rising gate, the register rotates, and the bit at the end is copied to the beginning with a probability of flipping. A great algorithm for controllable randomness, as you can lock loops you like, or let them slowly evolve.
    - Slider/CV: Probability of flipping the copied bit. At 0, the sequence will be locked. At 1, the copied bit will always flip, allowing for locked sequences of twice the length. At 0.5, the copied bit will be completely random.
    - Pot: Number of steps, from 1 to 16.
    - Button + slider: Quantization scale, same as with step and hold segments.
    - Button + pot: Polarity. Unipolar ranges between 0v and 8v. Bipolar between -5v and 5v.
- Ungated, looping: Random LFO with variable shape and frequency
    - Slider/CV: Frequency in V/oct. Frequency range is ~8 secs to ~32hz.
    - Pot: Smoothness: At full CCW, output will be stepped, uniformly random. This will increasingly smooth up until 12:00, using sine curve-like transitions between points. After 12:00, the output will transition to using a random walk to generate outputs instead. Increasing the pot further will decrease the variance of each step, resulting in small wiggles at full CW.
    - Button + slider: Frequency range:
        - Slow: 2 min to 2hz
        - Medium: 8 secs to 32hz at the middle
        - Fast: 32hz to ~8khz; when the segment exceeds about 8khz, it will switch to full noise with the pot controlling color.
    - Button + pot: Polarity. Unipolar ranges between 0v and 8v. Bipolar between -5v and 5v.
    - This video shows the range of behaviors: https://www.youtube.com/watch?v=SHhhvDU7lb4
- Gated, looping: Clocked random LFO
    - Just like random LFOs as described above, but clocked.
    - Slider/CV: Clock division/multiplication.
    - Pot: Smoothness using same shapes as unclocked random LFO.
    - Button + slider: Division/multiplication range, just as with regular LFOs:
        - Slow: 1/32, 1/16, 1/8, 1/7, 1/6, 1/5, 1/4, 1/3, 1/2, 1
        - Medium: 1/4, 1/3, 1/2, 1, 2, 3, 4 
        - Fast: 1, 2, 3, 4, 5, 6, 7, 8, 12, 16
    - Holding the segment's button while you patch the gate input will cause the random LFO to reset on triggers instead of clocking.

When used in an envelope, a **non-looping** random segment will act like a step (orange) segment with a Turing Machine as input.
When the envelope reaches the random segment, the envelope will output the value of the segment's shift register and hold until the envelope receives another trigger.
When the envelope receives the trigger, the envelope will move on from the random segment, and the segment will advance its shift register as described above.

A **looping** random segment will act like a looping hold (red) segment with a Turing Machine as input.
When the envelope reaches the random segment, the envelope will output the value of segment's shift register until the envelope's gate goes low, at which point the segment's shift register will advance.

##### Single, non-gated, non-looping ramp (green) segments slew with independent rise and fall

In normal Stages, single, non-gated, non-looping ramp segments didn't do anything.
In advanced mode, they act as slew segments with independent rise and fall control.
Slider controls fall time (just like in single, gated, non-looping ramp segments) and pot controls rise time.
CV input gives the target value.
They have the same natural RC curve of slewed yellow segments (concave rise and convex fall), making them natural for use as audio envelopes when paired with a linear VCA.
These segments have many uses beyond standard slew:

- ASR envelopes: Patch your gate into the *CV* input (not gate). When the gate is high, the segment will rise and hold until the gate is release, at which point the segment will fall. This can be a nice alternative to decay segments (single, gated, non-looping green) when you want to avoid a clicky attack, or can be used as a general purpose ASR.
- Envelope follower: Patch an audio signal into CV. Set rise short and fall longer. The segment will follow the amplitude of the audio signal. Adjust fall time to taste. Flip rise and fall to create an inverted envelope follower.

Button+pot changes polarity. In unipolar mode (default), a full-wave rectifier is applied to the incoming signal, improving behavior as an envelope follower. In bipolar mode, no rectifier is applied.

Button+slider changes the time range of the slider, similar to that of a gate ramp segment, with long time range at the top, default time range in the middle, and short time range at the bottom.

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
Feeding in an LFO gives you arpeggios with a controllable range.
Feeding in a TM segment gives a generative melody.

##### Single, gated, looping hold (red) segments have probability control

The pot on single, gated, looping hold segments now controls their probability of firing.
Previously, the pot did not do anything.
Fully CW will cause it to always fire (the original behavior) and fully CCW will cause it to never fire.
Note that this feature was added into the official firmware in v1.2.

### Six independent DAHDSR envelope generators
The module transforms into a generator of six independent envelopes.

Only one of the envelopes is considered selected at any time. Changes to the **slider** positions will only affect the selected envelope. Pressing a **button** will switch which envelope is selected. When switching the selected envelope, all **sliders** become disabled (led off) to avoid modifying the envelope values. Moving a slider will enable it (and its corresponding pot), updating the value of the corresponding stage in the selected envelope. Pressing the **button** of the selected envelope will immediately activate all **sliders** (and pots). Setting the **sliders** for one envelope, selecting another envelope, and then pressing the button again will effectively duplicate the settings onto the second envelope.

**Sliders** controls the duration (or level) of each stage of all envelopes. From left to right:

1. Duration of the **delay** phase
2. Duration of the **attack** phase
3. Duration of the **hold** phase
4. Duration of the **decay** phase
5. Level of the **sustain** phase
6. Duration of the **release** phase

Each duration goes from 0 to 10 seconds. The hold phase is always at maximum level (8V). Each stage can be "disabled" by setting the slider to the bottom. For example, set sliders 1 and 3 to zero to get a standard ADSR envelope.

**SHAPE/TIME** pots 2, 4 and 6 control the **shape** of the corresponding ramp stages, from accelerating through linear, to decelerating. Each pot is only enabled for the current envelope if the corresponding slider is enabled for the current envelope (slider led is lit green). Pots 1, 3 and 5 are unused.

**GATE** inputs are used to activate each of the six envelopes, which can be taken from the corresponding outputs on the bottom of the module. LEDs below pots show the current phase of each envelope: green for delay/attack/hold/decay, orange for sustain, red for release. The selected envelope will be yellow when idle while the inactive envelopes will be off when idle.


### Six identical DAHDSR envelope generators

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

You can adjust the harmonic range of each segment by the segment's button and moving its slider to the top, middle, or bottom.
The harmonic ranges are:

- Top: 1x, 2x, 3x, ..., 16x
- Middle (default): 1/4x, 1/2x, 1x, 1.5x, 2x, 3x, 4x, 5x, 6x, 8x
- Bottomw: 1/16x, 1/15x, 1/14x, ..., 1x

### Harmonic oscillator with alternate controls

Same as harmonic oscillator, but controls for each partial (columns 2 to 6) are swapped:

- Pot controls the **volume** in the mix
- Slider and CV input sets the **harmonic ratio** in relation to the root pitch

This way it is possible to modulate (and therefore sequence) the harmonics with external CV.

The frequency range in this mode can be controlled just as with the normal harmonic oscillator mode (hold leftmost button and move leftmost slider).

Other changes
-------------

The original firmware would save state when a button was released.
This firmware saves state 5 seconds after a state change occurs, reseting the the timer if another change occurs within that time.
Because this firmware uses significantly more flash storage to save state, this change reduces the strain on flash in order to extend the life of the chip.
All LEDs momentarily blink off when a save occurs.

Known issues
------------

- **Having too many audio-rate clocked oscillators result in clicks and pops.** The audio-rate oscillators introduced in the official v1.2 firmware are somewhat computationally intense, as is the PLL algorithm used for clocking. Thus, when you have too many clocked oscillators, segments cana struggle to keep up, produces clicks and pops or audio distortion in the 5th or 6th segment. Currently, four clocked oscillators can be comfortably run simultaneously. Note that this issue does not affect audio-rate LFOs: you can run 6 clocked audio-rate LFOs simultaneously, and they will be bandlimited and thus sound decent at audio-rate.

See https://github.com/qiemem/eurorack/issues for full list of issues.

Building
--------

The typical build process for Mutable Instruments firmware involves building a virtual machine via Vagrant using the [Mutable Dev Environment](https://github.com/pichenettes/mutable-dev-environment).
However, I have started building this fork using a more up-to-date version of the Arm GNU Toolchain and have seen improved performance of the firmware as a result.
Thus, if you are interested in building the firmware yourself, I would recommend this approach (for Linux or MacOS):

1. Download the latest release of the [bare-metall (arm-none-eabi) Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) for your platform. I use the bundles from there rather than from a package manager as the folder structure matches what the Mutable makefile expects. I currently use version 13.2.rel1.
2. In the `eurorack-modules` directory, run

```
TOOLCHAIN_PATH=/path/to/arm-gnu-toolchain-x.x.x-platform-arm-none-eabi/ make -f stages/makefile wav
```

or add `FLIPPED=true` to build the flipped version of firmware for using the module upside down:

```
TOOLCHAIN_PATH=/path/to/arm-gnu-toolchain-x.x.x-platform-arm-none-eabi/ FLIPPED=true make -f stages/makefile wav
```

3. Install the built firmware via the [standard procedure](https://pichenettes.github.io/mutable-instruments-documentation/modules/stages/manual/#firmware). You will find the built wav files in `build/stages/stages.wav` or `build/stages-flipped/stages-flipped.wav`. I use `aplay` to do this like so:

```
aplay -V mono build/stages/stages.wav
```

Changelog
---------

Fork:

- [v1.3.0](https://github.com/qiemem/eurorack/releases/tag/v1.3.0)
    - Merge all behavior from the final [v1.2](https://pichenettes.github.io/mutable-instruments-documentation/modules/stages/firmware/) of the official firmware.
    - Fix clocked LFO performance issues to allow for six clocked LFOs.
    - Add bandlimiting to audio-rate LFOs to reduce aliasing on the original LFO shapes.
    - Make slow LFO mode correctly chain with standard mode, not advanced mode.
    - Add flipped build for using Stages upside down. Huge thank you to James Everitt for implementing the driver changes and to chadwick for help testing!
    - Make LFOs resettable by holding the button when patching the gate input.
- [v1.2.2](https://github.com/qiemem/eurorack/files/10714684/stages-bipolar-v1.2.2.zip) (never officially released)
    - [Visual cheatsheet](https://raw.githubusercontent.com/qiemem/eurorack/bipolar/stages/resources/Stages%20Qiemem%20Cheatsheets.pdf) added by Philip Belesky! #44.
    - Rework random segments to use proper, variable-shape random LFO behavior. Suggested by breach on the old MI forum (pour one out). #3
    - Allow changing harmonic ranges in ourobor mode. Suggested by [Hanz](https://www.muffwiggler.com/forum/viewtopic.php?f=16&t=235787&start=300#p3365247) (#21) and nifgam in #37.
    - Make trig to gate 1 in ouroboros mode reset LFO phases.
    - Make extended sequencer in addressable mode hold on clocks.
    - Add quantization to digital shift registers.
    - Add LED flash when changing a digital shift register length or quantized note value.
    - Make shift registers advance at end of succeeding step in a group to prevent value suddenly changing while affecting other segments.
    - Only save on button release to reduce strain on flash and interruptions while adjusting button+control params.
    - Remove codebook from quantizer to reduce memory usage.
    - Make sure looping yellow segments track correct value when used in a multi-module group. #38
    - Ensure changing loop status triggers a save. #33
    - Lots of optimizations
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

