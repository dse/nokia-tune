#!/bin/bash
#
# I'll make this easier eventually.

# type is one of sine, square, triangle, sawtooth, trapezium, exp,
# [white]noise, tpdfnoise, pinknoise, brownnoise, pluck; default=sine.
type=exp
gain=0

typex=""
gainx=""

for i ; do
    case "$i" in
        sine|square|triangle|sawtooth|trapezium|exp|pluck)
            type="$i"
            typex="-$i"
            ;;
        [0-9]*)
            gain="-$i"
            gainx="-$i"
            ;;
    esac
done

eighth=0.125
delay=0.25
quarter="$(echo $eighth \* 2 | bc -l)"
half="$(echo $quarter \* 2 | bc -l)"
threequarters="$(echo $quarter \* 3 | bc -l)"
remainder="$(echo 6 - $eighth \* 24 - $delay | bc -l)"

tmpdir="$(mktemp -d /tmp/nokiatune.XXXXXXXXXX)"

sox -q -n $tmpdir/00.wav synth $delay         $type A5  gain -1000
sox -q -n $tmpdir/01.wav synth $eighth        $type E6  gain "${gain}"
sox -q -n $tmpdir/02.wav synth $eighth        $type D6  gain "${gain}"
sox -q -n $tmpdir/03.wav synth $quarter       $type F#5 gain "${gain}"
sox -q -n $tmpdir/04.wav synth $quarter       $type G#5 gain "${gain}"
sox -q -n $tmpdir/05.wav synth $eighth        $type C#6 gain "${gain}"
sox -q -n $tmpdir/06.wav synth $eighth        $type B5  gain "${gain}"
sox -q -n $tmpdir/07.wav synth $quarter       $type D5  gain "${gain}"
sox -q -n $tmpdir/08.wav synth $quarter       $type E5  gain "${gain}"
sox -q -n $tmpdir/09.wav synth $eighth        $type B5  gain "${gain}"
sox -q -n $tmpdir/10.wav synth $eighth        $type A5  gain "${gain}"
sox -q -n $tmpdir/11.wav synth $quarter       $type C#5 gain "${gain}"
sox -q -n $tmpdir/12.wav synth $quarter       $type E5  gain "${gain}"
sox -q -n $tmpdir/13.wav synth $threequarters $type A5  gain "${gain}"
sox -q -n $tmpdir/14.wav synth $remainder     $type A5  gain -1000

dir="sounds"
basename="nokiatune${typex}${gainx}"

mkdir -p "${dir}"

sox $tmpdir/*.wav "${dir}/${basename}".wav

ffmpeg -y -i "${dir}/${basename}".wav "${dir}/${basename}".mp3
ffmpeg -y -i "${dir}/${basename}".wav "${dir}/${basename}".m4a
ffmpeg -y -i "${dir}/${basename}".wav "${dir}/${basename}".flac
cp "${dir}/${basename}".m4a "${dir}/${basename}".m4r

/bin/rm -f -r $tmpdir
