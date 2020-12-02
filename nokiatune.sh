#!/bin/bash
#
# I'll make this easier eventually.

type=exp
eighth=0.125
delay=0.25
quarter="$(echo $eighth \* 2 | bc -l)"
half="$(echo $quarter \* 2 | bc -l)"
threequarters="$(echo $quarter \* 3 | bc -l)"
remainder="$(echo 6 - $eighth \* 24 - $delay | bc -l)"

tmpdir="$(mktemp -d /tmp/nokiatune.XXXXXXXXXX)"

sox -q -n $tmpdir/00.wav synth $delay         $type A5  gain -1000
sox -q -n $tmpdir/01.wav synth $eighth        $type E6  gain -2
sox -q -n $tmpdir/02.wav synth $eighth        $type D6  gain -2
sox -q -n $tmpdir/03.wav synth $quarter       $type F#5 gain -2
sox -q -n $tmpdir/04.wav synth $quarter       $type G#5 gain -2
sox -q -n $tmpdir/05.wav synth $eighth        $type C#6 gain -2
sox -q -n $tmpdir/06.wav synth $eighth        $type B5  gain -2
sox -q -n $tmpdir/07.wav synth $quarter       $type D5  gain -2
sox -q -n $tmpdir/08.wav synth $quarter       $type E5  gain -2
sox -q -n $tmpdir/09.wav synth $eighth        $type B5  gain -2
sox -q -n $tmpdir/10.wav synth $eighth        $type A5  gain -2
sox -q -n $tmpdir/11.wav synth $quarter       $type C#5 gain -2
sox -q -n $tmpdir/12.wav synth $quarter       $type E5  gain -2
sox -q -n $tmpdir/13.wav synth $threequarters $type A5  gain -2
sox -q -n $tmpdir/14.wav synth $remainder     $type A5  gain -1000

sox $tmpdir/*.wav nokiatune.wav
sox $tmpdir/*.wav nokiatune.mp3

ffmpeg -i nokiatune.wav nokiatune.m4a
cp nokiatune.m4a nokiatune.m4r

/bin/rm -f -r $tmpdir
