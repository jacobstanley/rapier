#!/bin/bash -eu

ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"

echo "Running from: $ROOT_DIR"
cd $ROOT_DIR

if [ -d staging ]; then
  echo "MT4 & Zorro already staged."
else
  mkdir -p staging
  echo "Staging MT4..."
  (cd mt4 && tar vcfj ../staging/mt4.tar.bz2 *)
  echo "Staging Zorro..."
  (cd zorro && tar vcfj ../staging/zorro.tar.bz2 *)
fi

TAG=rapier

docker build --tag $TAG .

docker run -it \
    --cap-add=SYS_PTRACE \
    -v "$(pwd)/store:/home/rapier/store" \
    -p 80:80 \
    -p 2222:22 \
    $TAG
