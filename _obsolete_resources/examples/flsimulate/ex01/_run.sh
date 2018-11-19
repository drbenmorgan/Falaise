#!/usr/bin/env bash

opwd=$(pwd)

which flsimulate > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo >&2 "[error] Falaise is not setup!"
    exit 1
fi

work_dir="$(pwd)/_work.d"
mkdir ${work_dir}
cp flsimulate.conf seeds.conf vprofile.conf ${work_dir}/
cd ${work_dir}/

echo >&2 "[info] Running flsimulate..."
flsimulate \
    --verbosity "trace" \
    --config "flsimulate.conf" \
    --output-metadata-file "flSD.meta" \
    --embedded-metadata=0 \
    --output-file "flSD.brio"
if [  $? -ne 0 ]; then
    echo >&2 "[error] flsimulate failed!"
    cd ${opwd}
    exit 1
fi

echo >&2 "[info] Running flvisualize..."
flvisualize \
    --variant-profile "vprofile.conf" \
    --input-metadata-file "flSD.meta" \
    --input-file "flSD.brio"

# Set tags:
# --detector-config-file "urn:snemo:demonstrator:geometry:4.0" \
# --variant-config "urn:snemo:demonstrator:geometry:4.0:variants" \

cd ${opwd}
echo >&2 "[info] The end."
exit 0

# end
