#!/usr/bin/env bash
# bin/opennlp SentenceDetector models/en-sent.bin < text | bin/opennlp TokenizerME models/en-token.bin

if [ $# -ne 1 ]
then
  echo "Error, missing input file"
fi

INSTALL_PATH=~/github/apache-opennlp-1.6.0-src/opennlp-tools
BIN=${INSTALL_PATH}/bin/opennlp
SENT_PATH=${INSTALL_PATH}/models/en-sent.bin
TOKEN_PATH=${INSTALL_PATH}/models/en-token.bin

${BIN} SentenceDetector ${SENT_PATH} < $1 | ${BIN} TokenizerME ${TOKEN_PATH}
