#!/bin/zsh

pdal=/Users/acbell/pdal/build/bin/pdal
percent=1
format=8
eb=3
count=0

while [ 1 ]
do
    echo $count
    ((count++))
    rm -f lazperf.laz laszip.laz lazperf.las laszip.las foo.1 foo.2

    if [[ $format == 0 ]]
    then
        pointsize=20
    elif [[ $format == 1 ]]
    then
        pointsize=28
    elif [[ $format == 2 ]]
    then
        pointsize=26
    elif [[ $format == 3 ]]
    then
        pointsize=34
    elif [[ $format == 6 ]]
    then
        pointsize=30
    elif [[ $format == 7 ]]
    then
        pointsize=36
    elif [[ $format == 8 ]]
    then
        pointsize=38
    else
        echo "Bad format $format"
        break;
    fi
    (( pointsize+=eb ))

    /Users/acbell/lazperf/build/tools/random foo.las $format/$eb $percent
    (( percent++ ))
    if (( percent > 20 ))
    then
        percent=1
    fi

    $pdal translate --writers.las.minor_version=4 --writers.las.dataformat_id=$format --writers.las.extra_dims=all --writers.las.compression=lazperf foo.las lazperf.laz  --writers.las.forward=all
    $pdal translate --writers.las.minor_version=4 --writers.las.dataformat_id=$format --writers.las.extra_dims=all --writers.las.compression=laszip foo.las laszip.laz  --writers.las.forward=all
    diff lazperf.laz laszip.laz
    res=$?
    if [[ $res != 0 ]]
    then
        echo "Bad encoding"
        break
    fi
#
    $pdal translate --writers.las.minor_version=4 --writers.las.dataformat_id=$format --writers.las.extra_dims=all --readers.las.compression=lazperf laszip.laz lazperf.las  --writers.las.forward=all
    $pdal translate --writers.las.minor_version=4 --writers.las.dataformat_id=$format --writers.las.extra_dims=all --readers.las.compression=laszip laszip.laz laszip.las  --writers.las.forward=all
    diff lazperf.las laszip.las
    res=$?
    if [[ $res != 0 ]]
    then
        echo "Bad decoding"
        break
    fi
#
    (( fullsize=50000 * pointsize ))
    tail -c $fullsize foo.las > foo.1
    tail -c $fullsize laszip.las > foo.2
    diff foo.1 foo.2
    res=$?
    if [[ $res != 0 ]]
    then
        echo "Bad comparison"
        break
    fi
done
