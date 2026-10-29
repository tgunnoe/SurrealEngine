# Datacube, book and newspaper runs

These runs open a readable Deus Ex device (a datacube, a book or a newspaper) in a stock map, to show what the engine hands the stock scripts when they read the device's text. The text is written in a small tag language (`<P>`, `<B>`, `<DC=r,g,b>`, `<JC>`, `<EMAIL=...>` and so on), and the scripts read it one token at a time from the engine's text parser. A centre tag (`<JC>`) must reach the script as the centre-justify token, so that the script aligns the line; the previous parser delivered it as a token the script ignores.

The tag language itself is covered by the unit tests under `tests/dx-text/`, which feed literal texts to the tokenizer. The colour of a text cannot be seen in a script trace (a trace prints a struct argument as `?`), so the tests are its only check, and they show only that the tokenizer reads the three numbers: the copy of that colour into the parser's `DefaultColor` and `LastColor` (in `UDXTextParser::ProcessText`) is read from the code, not shown by a test or a run. What a run adds is the whole path: a frobbed device, the stock `InformationDevices` script, the parser's natives and the text window that receives the token.

`Previous binary` is an engine built before the tokenizer was changed; `Fixed binary` is an engine built from the change that makes it deliver the justification tags (and the colour tag, the five-field email tag and the file tag that the tests check).

## Recipe

Every run is an unattended smoke run, started with the same flags and no `--user-dir` (the engine then writes no user files and nothing into the game folder):

```
SurrealEngine --autostart --exit-after=60 --smoke-log=<log> --url=12_Vandenberg_Computer \
    --exec='<items>' --trace-script=<entries> "<game folder>"
```

Tools/smoke/README.md describes the flags, `hold`, `call` and the trace. What these runs add:

- The trace entries are `InformationDevices.CreateInfoWindow`, `InformationDevices.ProcessTag`, `DeusExTextParser.OpenText`, `DeusExTextParser.ProcessText`, `DeusExTextParser.GetTag`, `TextWindow.SetTextAlignments`, `TextWindow.SetTextColor`, `TextWindow.SetText` and `TextWindow.AppendText`. The engine log (`SE-Log-LastRun.txt`, written when the engine exits) is read from the line of the `CreateInfoWindow()` entry on; the lines before it are the HUD building its own windows at login.
- A device is frobbed with `call FrobTarget.Frob self none`, which frobs what the player's view points at, within the player's frob reach. A second frob closes the reading window, so each run frobs once.
- The player's position is probed with `call LineOfSightTo class:JCDentonMale` every 0.5 s.
- `call FrobTarget.IsA InformationDevices` returns `True` while the view points at a device and logs `refused: FrobTarget is None` while it points at nothing. Run every 0.1 s during a slow `hold axis aLookUp` it finds the window of pitch in which the device is the frob target (see below).
- The runs are not deterministic: the player ends a few units from run to run, which the aim tolerates.

## Book in the Vandenberg computer room

| Map | Device | Text | Tags in the text | Previous binary | Fixed binary |
| :--- | :--- | :--- | :--- | :--- | :--- |
| 12_Vandenberg_Computer | BookOpen2 | 12_Book01 | 1 `DC`, 14 `P`, 1 `B`, 1 `/B`, 3 `JC`; 28 tokens | 1 `CreateInfoWindow()`, 28 `ProcessTag(`, 15 `SetTextAlignments(0, 0)`, no `SetTextAlignments(1, 1)` | 1 `CreateInfoWindow()`, 28 `ProcessTag(`, 15 `SetTextAlignments(0, 0)` and 3 `SetTextAlignments(1, 1)`, one per `JC` |

`SetTextAlignments(0, 0)` is the window script aligning every new window (the first token and each `<P>` start one: 1 + 14 = 15); `(1, 1)` is the stock `InformationDevices.ProcessTag` aligning a window to the centre, which only a delivered centre tag causes. `SetText(` appears 8 times and `AppendText(` never in both, and `SetTextColor(?)` 16 times in both (the colour is not visible in the trace).

Two runs per binary, in alternation, gave the same counts and pairs.

The choreography (the start of this map has the player facing east, in a room whose desk carries the book at the far side):

```
5:hold axis aTurn 2244 1;6.3:hold axis aForward 300 2.302;9.4:hold axis aTurn -2244 1;10.7:hold axis aForward 300 1.4;12.6:hold axis aTurn -1390 1;14:hold axis aLookUp -200 2.9;17.4:call FrobTarget.Frob self none
```

plus the probes above, `--exit-after=60`, and the trace entries listed in the recipe.

The player turns the view by +76.7 degrees and walks about 340 units to the open floor north-west of the desk, turns back by -76.7 degrees to face east and walks into the face of the solid block east of that floor (which stops it at the same x in every run), then turns -47.5 degrees toward the book and pitches the view down for 2.9 s at an `aLookUp` of -200. A `hold` of `aTurn` changes the view by about 17.5 degrees per 512 of axis value held for one second (the turns above are computed from that); the pitch is about 20 degrees if `aLookUp` acts at the same rate. The frob follows 0.5 s after the pitch ends, which is long enough for the player's frob target to refresh (it refreshes ten times a second).

How the stand point and the aim were found: the book's position, the player start and the level geometry were read from the map file offline. The book lies on a desk top 51 units above the floor, higher than the player can step; the run stands on the open floor north-west of the desk, about 99 units from the book on the floor plan. A first run walked there and swept the view (down, up, down, up, down at `aLookUp` -200 for 4 s each, with an `aTurn` of 3 degrees between sweeps) while probing `FrobTarget.IsA InformationDevices` every 0.1 s: the device was the frob target for up to 0.7 s of a sweep, 2.55 to 3.25 s into a downward sweep, so a 2.9 s pitch aims at the middle of that window.

## Why this map, and what the run does not cover

The three readable devices of `00_Training` (`DataCube0`, `DataCube2` and `BookOpen0`) are all beyond the locked door pair `DeusExMover45` and `DeusExMover46` (`KeyIDNeeded` `FrontDoor`, tag `gundoor`), which stands on the only route to them from the player start; the training script opens it, not the player. With that door pair left shut, a walkability model built from the map's collision hulls reaches none of the three. The readable devices of `01_NYC_UNATCOIsland` are more than 8000 units from its player start, so neither map suits a short unattended run. In `12_Vandenberg_Computer` the book is about 340 units from the start on the floor plan.

Datacubes and newspapers run through the same script class as the book (`InformationDevices` is the parent of `DataCube`, `BookOpen`, `BookClosed`, `Newspaper` and `NewspaperOpen`) and the same parser, but none was opened in a run: this scenario covers a book only.

The email and file tags belong to the computer screens' menu texts, and no run opened a computer screen. The unit tests show the tokenizer reading file tags and five-field email tags (`<EMAIL=name,subject,from,to,cc>`). A four-field email tag (and an empty `<EMAIL=>`) is still misread: when no comma follows it in the text it falls back to text, and otherwise it reads on past its `>` into the next email tag, so that email is lost. That gap is not covered by the tests or by this scenario.
