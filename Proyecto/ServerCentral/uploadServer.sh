#!/bin/bash

if [ $# != 1 ]; then
  echo `basename $0`: 1 parametro necesario, debe ser el mismo puerto que el servidor
  exit 1
fi

ngrok http --scheme=http http://localhost:$1 --domain morally-alert-polecat.ngrok-free.app
