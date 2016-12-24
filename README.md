#KhedNES
An NES emulator built on top of SDL2. Honestly, there are better ones out there. This one is kind of crap. I just want to store a copy online "Just In Case".

Bonus: Now there's some chance that it'll play an NSF if you give it one. DMC (digitized audio) seems to be pretty broken in a lot of cases, but as long as the game isn't one of the Japanese ones that contained extra audio chips in the cartridge, it should play.

##Emulator keys
A+B are k and l
start+select are g and h
direction pad: wasd
quit: q
z and x do some manipulation of the zapper (gun accessory), but aren't too useful beyond testing.
debug view: tab. It shows the state of the 4 name tables at the end of the frame, the hi+lo priority sprites, the pattern tables, and the palettes.
dump name table: n. It prints out the actual values in PPU memory that define which tiles the background should use.
number keys: activate and deactivate various debug outputs, which I've got mostly commented out anyhow, because they're pretty bad for performance, even when I skip printing them out.
pause: p
reset: r

##NSF player keys
previous and next: a and d
quit: q
reset: r (restart the song)
