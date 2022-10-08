#!/bin/bash
#
# I'll make this easier eventually.

ffmpeg="ffmpeg -y -loglevel fatal"

# type is one of sine, square, triangle, sawtooth, trapezium, exp,
# [white]noise, tpdfnoise, pinknoise, brownnoise, pluck; default=sine.
type=sawtooth
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
rm -f -r "${dir}"
mkdir -p "${dir}"

sawtoothwav="${dir}/nokiatune${typex}${gainx}.wav"
sawtoothoutwav="${dir}/nokiatune${typex}${gainx}.out.wav"
wav="${dir}/nokiatune${gainx}.wav"
mp3="${dir}/nokiatune${gainx}.mp3"
m4a="${dir}/nokiatune${gainx}.m4a"
m4r="${dir}/nokiatune${gainx}.m4r"
flac="${dir}/nokiatune${gainx}.flac"

set -x

sox $tmpdir/*.wav "${sawtoothwav}"
src/wave "${sawtoothwav}"
mv "${sawtoothoutwav}" "${wav}"

$ffmpeg -i "${wav}" "${mp3}"
$ffmpeg -i "${wav}" "${m4a}"
$ffmpeg -i "${wav}" "${flac}"
cp "${m4a}" "${m4r}"

/bin/rm -f -r $tmpdir
