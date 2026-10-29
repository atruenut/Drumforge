# DrumForge User Guide

## 1. What it is, and when to use it

DrumForge is a synthesised drum kit instrument. Because nothing is sampled, every piece can be tuned, damped, drenched or destroyed, and you get no round-robin repetition: each hit gets its own noise seed plus a little humanisation. Use it when you want:

* a tweakable acoustic-style kit (rock, jazz, funk, metal) without a multi-GB sample library;
* electronic kits (808, 909, trap, DnB) with real control over each sound;
* a quick groove sketch: the built-in player plays 28 patterns in sync with your DAW;
* cinematic, industrial or strange percussion from the Experimental / Ambient presets.

Honest scope: the acoustic sounds are physically-inspired synthesis. They are punchy and flexible, but they are not recordings of a real kit, so expect "great synthetic kit", not "indistinguishable from Superior Drummer".

## 2. Install

1. Build (see README) or copy the supplied `DrumForge.vst3` folder to `C:\Program Files\Common Files\VST3\`.
2. A single-file `DrumForge.vst3` (the DLL) dropped straight into that folder also works in most hosts.
3. Needs the Microsoft Universal C Runtime (built into Windows 10/11).
4. Rescan plugins in your DAW. It appears as an **Instrument** (category Instrument|Drum) named DrumForge.

## 3. Playing it

Route a MIDI track (or a MIDI drum clip) to DrumForge. The note map follows General MIDI:

| Note | Sound | Note | Sound |
|---|---|---|---|
| 35 | Kick 2 | 36 | Kick |
| 37 | Sidestick | 38 | Snare |
| 39 | Clap | 40 | Snare rimshot |
| 41 | Floor tom | 43 | Floor tom hi |
| 45 | Mid tom | 47 | Mid tom hi |
| 48 | High tom lo | 50 | High tom |
| 42 | Closed hat | 44 | Pedal hat |
| 46 | Open hat (choked by closed / pedal) | 49 | Crash |
| 57 | Crash 2 | 55 | Splash |
| 52 | China | 51 | Ride |
| 59 | Ride 2 | 53 | Ride bell |
| 54 | Tambourine | 56 | Cowbell |
| 63 | Conga hi | 64 | Conga lo |
| 70 | Shaker | 75 | Claves |
| 80 | Triangle (muted) | 81 | Triangle |

Velocity changes level, brightness and attack together. **Vel Curve** (Master) bends the response; **Note Shift** moves the whole map if your drum clips use a different octave.

## 4. The editor (Windows)

* **Header:** **SAVE** / **LOAD** (your own kit files), preset name with prev / next arrows and a list popup (click the name), the **MUTATE** button, the **MORPH** knob and the Master Volume knob.
* **Piece buttons** (left): choose Kick, Snare, Sidestick, three Toms, Hi-hat, Crash, Ride or Perc. Each has Mute / Solo and a flashing hit meter.
* **Piece panel:** common controls (Level, Pan, Tilt, Drive, Dynamics, Room Send, **Trans Attack**, **Trans Sustain**) plus that piece's own sound controls.
* **Test pads** (bottom of the piece panel): one pad for every sound of the selected piece - kick 1/2, snare / rimshot, closed / pedal / open hat, crash / china / splash, ride / bell, every percussion sound and so on. Click a pad to hear it; **clicking higher on the pad plays harder**, Shift-click plays a soft ghost note. Clicking a drum in the kit list also plays it. Test pads ignore Note Shift and work with the transport stopped.
* **Tabs (bottom):** MIX FX (EQ, Bus Comp, Saturator), ROOM, GROOVE (player, feel, lane mutes, the Groove Editor button) and MASTER (Humanize, Vel Curve, Width, Master Tune, Note Shift, Limiter, Snare Buzz, Morph Amount, Morph To).
* **Mouse:** drag a knob vertically; hold Shift for fine control; mouse wheel also works; double-click or right-click resets to default. Escape closes the preset popup; left/right arrows step presets.
* Without the GUI (Linux/macOS or a host that hides it) every control is still a host parameter, with readable values.

## 5. Concepts

* **Level / Pan / Tilt / Drive / Dynamics / Room Send** exist on every piece. *Tilt* is a one-knob tone control (dark to bright). *Dynamics* sets how much velocity changes the sound; 0 makes every hit identical. *Room Send* feeds the shared room.
* **Humanize** (MASTER tab) adds small random variation to each hit's velocity, tuning and decay (every hit also gets its own noise). **Groove Human** adds timing and velocity variation to the groove player only.
* **Room:** an 8-line feedback-delay-network reverb with six types (Studio, Tight Booth, Live Hall, Cathedral, Gated, Plate), pre-delay, damping, size, decay and a gate time for 80s gated snares.
* **Master chain:** per-piece tilt, master EQ, saturator, bus compressor, then a look-ahead limiter. With the limiter on, output never exceeds the Ceiling.
* **Stems:** enable the aux outputs in your DAW to receive Kick, Snare, Toms, Hats, Cymbals, Perc and Room on separate stereo tracks. They are delay-matched to the main mix.
* **Latency:** the limiter adds about 1.5 ms. DrumForge reports it to the host.

### Saving and loading your own kits

**SAVE** (top left of the editor) writes every kit setting - all drum sounds, levels, pans, tilt, drive, transient shaper, EQ, compressor, saturator, room, Snare Buzz, humanize, width and so on - to a small text file (`.dfkit`). It opens a Save dialog in `Documents\DrumForge Kits`. **LOAD** opens a kit file and applies it. Groove settings, your 8 user grooves and Morph are not part of a kit, so loading a kit never disturbs your groove. Kit files are plain text (`key=value` per line), so you can share them or keep them in version control.

### Transient shaper, snare buzz, mutate and morph (new in 1.1)

* **Trans Attack / Trans Sustain** (every piece, -100..+100 %): a transient shaper that works on each drum separately. Attack boosts or softens the first few milliseconds of every hit (up to about +/-6 dB), Sustain lengthens or tightens the tail. Use Attack + on a kick for beater click, Sustain - on a kick to tighten a boomy room, Sustain + on a snare for a longer ring.
* **Snare Buzz** (MASTER tab): sympathetic resonance. Low drums (kick and toms) make the snare wires rattle, like a real snare drum in a real room. 0 = off, about 25-45 % is realistic, 100 % is exaggerated. Several presets use it.
* **MUTATE** (header): nudges the sound controls of every drum by a random amount (Shift or right-click: bigger) and plays the drum that is currently selected so you hear the result. Hit it a few times to find happy accidents; use your host's undo (or reload the preset) to go back.
* **MORPH** (header knob, MASTER tab, mod wheel): blends your whole kit toward another preset. *Morph To* picks the target preset, *Morph* is how far (0 = your kit, 100 = the target). Everything morphs smoothly - tunings, decays, EQ, room, saturation - except switches (they flip at 50 %), Mute / Solo, the limiter, Note Shift and the groove settings. **MIDI CC1 (mod wheel) controls Morph**, so you can morph live or automate it. Example: start from Studio Rock, set Morph To = Trap Sub Kit and draw a Morph automation lane from 0 to 100 across a build.

## 6. Groove player

Turn **Groove Player** on, choose a **Pattern** and press play in your DAW. It follows the host tempo and position. Played notes and recorded MIDI mix in on top of the pattern. The GROOVE tab has:

* **Pattern** - 28 factory patterns plus **User 1-8**, your own grooves from the Groove Editor.
* **Swing**, **Intensity** (how much of the pattern's softer detail plays), **Rate** (1/16, 1/8, 1/32), **Groove Human** (timing looseness) and **Fill Every** (a fill every 1, 2, 4 or 8 cycles).
* **Play When Stopped** - audition with the transport stopped (free-runs at the host tempo, or 120 BPM if the host gives none).
* **Laid-Back** (-40..+40 %) - delays (+) or pushes (-) the backbeat (snare, sidestick, clap) against the kick and hats. +10 to +20 % gives the classic behind-the-beat feel.
* **Lane mutes** (KICK, SNARE, HATS, TOMS, CYMB, PERC) - lit means that part of the pattern is muted. Mute the kick to hear what the pattern does without it, or mute the cymbals to play your own.
* A small **pattern preview** with a moving playhead.

**No surprise crashes.** The groove player never adds crash cymbals of its own. Crashes you place yourself in the Groove Editor always play.

**Looping.** The groove player is loop-aware: it never plays past the loop end, never doubles or drops a hit at the loop boundary, and gently fades any ringing crash / ride when the transport jumps (loop restart, locate, play after stop) so tails do not pile up under the new bar. A cymbal you hear exactly at the loop start comes from your own MIDI clip or the ride pattern.

### The Groove Editor

Click **GROOVE EDITOR** on the GROOVE tab to build your own grooves. Eight slots (**USER 1-8**) are saved with your project.

* **Slot buttons** - pick a slot; it also becomes the selected groove pattern, so you hear it right away (switch **GROOVE ON** and **PLAY WHEN STOPPED** in the editor if needed). A white column shows the playhead.
* **Grid** - 14 lanes (kick, snare, sidestick, clap, closed / open / pedal hat, floor / mid / high tom, crash, ride, ride bell, tambourine) by up to 64 steps.
  * **Click** an empty cell to place a hit (you hear it); **click** a hit to remove it.
  * **Drag across** empty cells to paint a run of hits.
  * **Drag a hit up or down** to set its velocity (the bar height shows it; very soft hits count as ghost notes).
  * **Right-click** a hit to make it a **ratchet**: 2, 3 or 4 rapid repeats inside that step (rolls, flams, trap hats). Dots show the count.
  * **Shift-click** a hit to set its **probability** (75 / 50 / 25 %). The thin white bar shows it; the hit then plays only some of the time, giving ever-changing variations.
  * **Click a lane name** to hear that drum.
* **LENGTH** - 8, 16, 24, 32, 48 or 64 steps (at 1/16 that is half a bar up to four bars). Odd lengths such as 24 make polymeter-style loops.
* **CLEAR**, **RANDOM** (a musical random groove to start from), **< SHIFT / SHIFT >** (rotate the whole pattern), **x2 LENGTH** (duplicate to double length, handy for building a variation in the second half).
* **COPY A FACTORY GROOVE** - choose any factory pattern with the arrows and press **COPY INTO SLOT** to start from it. Shaker / cowbell / claves parts land on the tambourine lane.
* Swing, Intensity, Groove Human, Laid-Back, Fill Every and lane mutes all apply to your grooves too. Rate sets the step size.

## 7. Worked example: a tight, modern rock chorus in 5 minutes

1. Create a MIDI track with DrumForge. Load **Rock: Studio Rock**.
2. Switch on the **Groove Player**, pick **Rock Driving**, set your project to 120 BPM and press play. You have a backing groove immediately.
3. Select **Snare**. Raise *Snap* to 60 for more crack, nudge *Tune* up a few Hz so it sits above the guitars, and set *Room Send* around 40 for a bigger backbeat.
4. Select **Kick**. Shorten *Decay* to about 300 ms and raise *Click* so it cuts through on small speakers. Add a touch of *Sub* if you want weight.
5. Open the **Hi-hat** piece: lower *Level* by 2 dB and pan it a little left, so it is not harsh.
6. Go to the **Room** tab: choose *Live Hall*, set *Room Mix* around 25% and *Decay* near 1.4 s.
7. In **Bus Comp**: Threshold -14 dB, Ratio 3, Attack 12 ms, Release 150 ms, Mix 60%. This glues the kit without flattening it.
8. Set **Fill Every** to *4 cycles* for turnarounds, and push **Laid-Back** to +10 % for a relaxed backbeat. Open the **Groove Editor**, copy Rock Driving into USER 1 and add a ratcheted ghost snare on the last 16th of each bar to make it yours. Optionally, record your own MIDI over the top of it with the notes from section 3.
9. Want it huge? Drag each stem to its own DAW track (enable the aux outputs) and add your own processing.
10. Save it in your DAW's preset system: your DAW stores the full DrumForge state with the project.

## 8. Tips

* Hear a sound before you tweak it: click its test pad (higher = harder).
* Tight, modern kick: **Trans Attack** +40, **Trans Sustain** -25.
* Realistic snare in a room: **Snare Buzz** 30 %.
* Live build: map the mod wheel (CC1) to Morph and morph from a tight kit to a huge one.
* Trap hats: in the Groove Editor right-click hats for 3x ratchets, and shift-click some for 50 % probability.
* Layer-free fat kick: *Sweep* 2.2 oct, *Sweep Time* 20 ms, *Sub* 50.
* 80s snare: *Room Type* Gated, *Gate Time* 220 ms, Room Send 70.
* Tight metal: Dynamics 100, *Drive* on kick and snare, Decay short, limiter on.
* Lo-fi: use Saturator at 40-60% and lower the Damping in the Room, or start from **Lo-Fi Dust** or **Broken Radio**.
* Mono compatibility check: set **Width** to 0.
* Sounds clipped or too quiet? Master Volume first, then limiter Ceiling.

## 9. Factory presets (40)

1. Rock: Studio Rock
2. Rock: Big Room Rock
3. Rock: Stadium Thunder
4. Rock: Garage Punk
5. Rock: Grunge Wall
6. Vintage: Vintage 60s
7. Vintage: Soul Ballad Warm
8. Jazz: Dry Jazz Kit
9. Funk: Funk Pocket Dry
10. Pop: Pop Tight
11. Dance: Disco Strut
12. Pop: Indie Dance
13. Metal: Death Metal Trigger
14. Metal: Thrash Clank
15. Metal: Black Metal Rawness
16. Metal: Djent Surgical
17. Metal: Doom Cavern
18. Metal: Grind Machine
19. Metal: Heavy Gallop
20. Electronic: 808 Sub Heavy
21. Electronic: 909 Classic
22. Electronic: Techno Warehouse
23. Electronic: Synthwave Gated
24. Electronic: DnB Rollers
25. Experimental: IDM Tuned Glitch
26. Hip-Hop: Trap Sub Kit
27. Hip-Hop: Boom Bap Dusty
28. Hip-Hop: Lo-Fi Dust
29. Cinematic: Taiko Hall
30. Cinematic: Trailer Hits
31. Cinematic: Apocalypse Toms
32. Industrial: Industrial Foundry
33. World: Tribal Skins
34. Latin: Latin Pocket
35. Reggae: Reggae One-Drop
36. Experimental: Metal Bin
37. Ambient: Cathedral Kit
38. Experimental: Underwater
39. Experimental: Rubber Room
40. Lo-Fi: Broken Radio

## 10. Groove patterns (28 factory + 8 user slots)

1. Rock: Rock Basic
2. Rock: Rock Driving
3. Pop: Pop Four-on-Floor
4. Rock: Half-Time Heavy
5. Soul: Motown Backbeat
6. Funk: Funk Pocket
7. Funk: Funk Sixteenths
8. Soul: Soul Ballad
9. Metal: Metal Gallop
10. Metal: D-Beat Thrash
11. Metal: Blast Beat
12. Metal: Death Double-Bass
13. Metal: Doom Crawl
14. Metal: Breakdown Chug
15. Electronic: House Four-on-Floor
16. Electronic: Techno Driver
17. Hip-Hop: Trap Half-Time
18. Electronic: DnB Two-Step
19. Hip-Hop: Boom Bap
20. Dance: Disco
21. Jazz: Jazz Swing Ride
22. Latin: Bossa Nova
23. Reggae: Reggae One-Drop
24. World: Tribal Toms
25. Cinematic: Taiko Pulse
26. Industrial: Industrial Stomp
27. Hip-Hop: Lo-Fi Shuffle
28. Pop: Indie Dance

## 11. Parameter reference

Every piece (Kick through Perc) has the same eight sound-independent controls, shown once under Kick: Level, Pan, Tilt, Drive, Dynamics, Room Send, Mute and Solo, plus the transient shaper (Trans Attack / Trans Sustain, shown under each piece). The piece-specific controls follow in each piece's table. Ranges and defaults come straight from the plugin's parameter table.

#### Master

| Parameter | Range | Default |
|---|---|---|
| Master Volume | -60 to 6 dB | 0 |
| Humanize | 0 to 100 % | 25 |
| Vel Curve | -100 to 100 % | 0 |
| Width | 0 to 150 % | 100 |
| Master Tune | -12 to 12 st | 0 |
| Note Shift | -12 to 12 int | 0 |
| Snare Buzz | 0 to 100 % | 0 |

#### EQ

| Parameter | Range | Default |
|---|---|---|
| Low | -12 to 12 dB | 0 |
| Mid | -12 to 12 dB | 0 |
| Mid Freq | 200 to 6000 Hz | 1000 |
| High | -12 to 12 dB | 0 |

#### Bus Comp

| Parameter | Range | Default |
|---|---|---|
| Threshold | -40 to 0 dB | -12 |
| Ratio | 1 to 20 :1 | 3 |
| Attack | 0.1 to 80 ms | 12 |
| Release | 20 to 800 ms | 150 |
| Makeup | 0 to 18 dB | 0 |
| Mix | 0 to 100 % | 0 |

#### Saturator

| Parameter | Range | Default |
|---|---|---|
| Drive | 0 to 100 % | 0 |
| Mix | 0 to 100 % | 100 |

#### Room

| Parameter | Range | Default |
|---|---|---|
| Room Mix | 0 to 100 % | 18 |
| Room Type | Studio / Tight Booth / Live Hall / Cathedral / Gated / Plate | Studio |
| Size | 0 to 100 % | 50 |
| Decay | 100 to 8000 ms | 1200 |
| Damping | 1000 to 20000 Hz | 7000 |
| Pre-Delay | 0 to 120 ms | 8 |
| Gate Time | 0 to 600 ms | 0 |
| Room Width | 0 to 100 % | 100 |

#### Limiter

| Parameter | Range | Default |
|---|---|---|
| Limiter | Off / On | 1 |
| Ceiling | -12 to 0 dB | -0.3 |

#### Kick

| Parameter | Range | Default |
|---|---|---|
| Level | -60 to 12 dB | 0 |
| Pan | -100 to 100 pan | 0 |
| Tilt | -12 to 12 dB | 0 |
| Drive | 0 to 100 % | 0 |
| Dynamics | 0 to 100 % | 70 |
| Room Send | 0 to 100 % | 12 |
| Mute | Off / On | 0 |
| Solo | Off / On | 0 |
| Tune | 30 to 120 Hz | 52 |
| Sweep | 0 to 4 oct | 1.8 |
| Sweep Time | 2 to 120 ms | 28 |
| Decay | 60 to 2000 ms | 420 |
| Punch | 0 to 100 % | 40 |
| Click | 0 to 100 % | 45 |
| Click Tone | 800 to 9000 Hz | 3500 |
| Sub | 0 to 100 % | 35 |
| Damp | 300 to 20000 Hz | 14000 |
| Attack | -100 to 100 % | 0 |
| Sustain | -100 to 100 % | 0 |

#### Snare

| Parameter | Range | Default |
|---|---|---|
| Tune | 100 to 400 Hz | 195 |
| Tension | 0 to 100 % | 30 |
| Body Decay | 40 to 800 ms | 160 |
| Body | 0 to 100 % | 70 |
| Wires | 0 to 100 % | 65 |
| Wire Decay | 60 to 900 ms | 260 |
| Wire Tone | 1500 to 9000 Hz | 4200 |
| Snap | 0 to 100 % | 45 |
| Shell | 0 to 100 % | 40 |
| Rimshot | 0 to 100 % | 15 |
| Damp | 500 to 20000 Hz | 16000 |
| Attack | -100 to 100 % | 0 |
| Sustain | -100 to 100 % | 0 |

#### Sidestick

| Parameter | Range | Default |
|---|---|---|
| Tune | 300 to 3000 Hz | 1100 |
| Decay | 10 to 300 ms | 70 |
| Click | 0 to 100 % | 50 |
| Attack | -100 to 100 % | 0 |
| Sustain | -100 to 100 % | 0 |

#### Floor Tom

| Parameter | Range | Default |
|---|---|---|
| Tune | 50 to 500 Hz | 92 |
| Decay | 80 to 2500 ms | 650 |
| Pitch Drop | 0 to 100 % | 35 |
| Drop Time | 5 to 150 ms | 40 |
| Attack | 0 to 100 % | 50 |
| Reso Head | 0 to 100 % | 40 |
| Damp | 400 to 20000 Hz | 12000 |
| Attack | -100 to 100 % | 0 |
| Sustain | -100 to 100 % | 0 |

#### Mid Tom

| Parameter | Range | Default |
|---|---|---|
| Tune | 50 to 500 Hz | 128 |
| Decay | 80 to 2500 ms | 520 |
| Pitch Drop | 0 to 100 % | 35 |
| Drop Time | 5 to 150 ms | 40 |
| Attack | 0 to 100 % | 50 |
| Reso Head | 0 to 100 % | 40 |
| Damp | 400 to 20000 Hz | 12000 |
| Attack | -100 to 100 % | 0 |
| Sustain | -100 to 100 % | 0 |

#### High Tom

| Parameter | Range | Default |
|---|---|---|
| Tune | 50 to 500 Hz | 175 |
| Decay | 80 to 2500 ms | 420 |
| Pitch Drop | 0 to 100 % | 35 |
| Drop Time | 5 to 150 ms | 40 |
| Attack | 0 to 100 % | 50 |
| Reso Head | 0 to 100 % | 40 |
| Damp | 400 to 20000 Hz | 12000 |
| Attack | -100 to 100 % | 0 |
| Sustain | -100 to 100 % | 0 |

#### Hi-Hat

| Parameter | Range | Default |
|---|---|---|
| Tune | -12 to 12 st | 0 |
| Closed Decay | 15 to 400 ms | 55 |
| Open Decay | 150 to 3000 ms | 700 |
| Bright | 3000 to 18000 Hz | 11000 |
| Metal | 0 to 100 % | 60 |
| 808 Blend | 0 to 100 % | 25 |
| Tick | 0 to 100 % | 40 |
| Ring | 0 to 100 % | 50 |
| Attack | -100 to 100 % | 0 |
| Sustain | -100 to 100 % | 0 |

#### Crash

| Parameter | Range | Default |
|---|---|---|
| Tune | -12 to 12 st | 0 |
| Decay | 300 to 6000 ms | 2200 |
| Bloom | 5 to 300 ms | 60 |
| Bright | 3000 to 18000 Hz | 12000 |
| Wash | 0 to 100 % | 40 |
| Tick | 0 to 100 % | 35 |
| Stereo | 0 to 100 % | 60 |
| Trash | 0 to 100 % | 15 |
| Attack | -100 to 100 % | 0 |
| Sustain | -100 to 100 % | 0 |

#### Ride

| Parameter | Range | Default |
|---|---|---|
| Tune | -12 to 12 st | 0 |
| Decay | 400 to 6000 ms | 2800 |
| Ping | 0 to 100 % | 55 |
| Bell | 0 to 100 % | 35 |
| Wash | 0 to 100 % | 25 |
| Bright | 3000 to 18000 Hz | 10000 |
| Stereo | 0 to 100 % | 40 |
| Dirt | 0 to 100 % | 10 |
| Attack | -100 to 100 % | 0 |
| Sustain | -100 to 100 % | 0 |

#### Perc

| Parameter | Range | Default |
|---|---|---|
| Tune | -12 to 12 st | 0 |
| Decay | 30 to 1500 ms | 150 |
| Spread | 0 to 100 % | 50 |
| Attack | -100 to 100 % | 0 |
| Sustain | -100 to 100 % | 0 |

#### Groove

| Parameter | Range | Default |
|---|---|---|
| Groove Player | Off / On | 0 |
| Pattern | 0 to 35 int | 0 |
| Swing | 0 to 100 % | 0 |
| Intensity | 0 to 100 % | 70 |
| Rate | 1/16 / 1/8 / 1/32 | 1/16 |
| Groove Human | 0 to 100 % | 10 |
| Fill Every | Off / Every cycle / 2 cycles / 4 cycles / 8 cycles | Off |
| Play When Stopped | Off / On | 0 |
| Laid-Back | -40 to 40 % | 0 |
| Mute Kick Lane | Off / On | 0 |
| Mute Snare Lane | Off / On | 0 |
| Mute Hat Lane | Off / On | 0 |
| Mute Tom Lane | Off / On | 0 |
| Mute Cymbal Lane | Off / On | 0 |
| Mute Perc Lane | Off / On | 0 |

#### Morph

| Parameter | Range | Default |
|---|---|---|
| Morph | 0 to 100 % | 0 |
| Morph To | Studio Rock / Big Room Rock / Stadium Thunder / Garage Punk / Grunge Wall / Vintage 60s / Soul Ballad Warm / Dry Jazz Kit / Funk Pocket Dry / Pop Tight / ... | Studio Rock |

## 12. Troubleshooting

* **Not in the plugin list:** make sure the folder structure `DrumForge.vst3\Contents\x86_64-win\DrumForge.vst3` (or the single file) is in `C:\Program Files\Common Files\VST3`, then rescan. Only 64-bit hosts are supported.
* **Silence:** check the host sends MIDI to the track, the piece is not muted or soloed elsewhere, and Master Volume is up.
* **Too loud or distorted:** lower Drive or Saturator; the limiter only guarantees the Ceiling, not taste.
* **Groove does nothing:** the host must be playing, or enable *Play When Stopped*.
* **Reporting bugs:** note the host, its version, the preset and the steps. The offline renderer (`df_render`) can reproduce engine problems without a host.

## 13. Known limits

* v1.1 changes the internal name of the Tilt control and adds parameters at the end of the list; projects saved with v1.0 still load (new controls start at their defaults).

* The Windows DLL was cross-compiled and checked for correct exports and dependencies, but has not been run inside a real Windows DAW by its author. Please report anything odd.
* The GUI exists on Windows only; Linux/macOS builds use the host's generic UI.
* The Groove Editor has fixed slot names (User 1-8) and no MIDI import / export yet.
* Not a sampled drum library: no round-robins, no mic bleed model, no multi-mic mixing.
