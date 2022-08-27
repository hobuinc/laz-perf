#!/bin/bash

set -e
cd "$(dirname "$0")"

mkdir -p lib/

npx tsc --project tsconfig.production.json

cp src/laz-perf.wasm lib/
cp src/laz-perf.d.ts lib/
cp src/laz-perf.js lib/

function bundle () {
	ENVIRONMENT="$1"

    # The laz-perf.js file with the bootstrapping code for each specific
    # environment must already exist so we don't need to account for that here.
    # The rest of these files are exactly the same for all environments, so
    # simply copy them into the directory for this environment.
	cp lib/index.js lib/${ENVIRONMENT}
	cp lib/index.d.ts lib/${ENVIRONMENT}
	cp lib/laz-perf.wasm lib/${ENVIRONMENT}
	cp lib/laz-perf.d.ts lib/${ENVIRONMENT}
}

bundle node
bundle web
bundle worker
