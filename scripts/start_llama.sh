#!/bin/sh
# Start llama-server with TinyLlama model

LLAMA_BIN=/usr/bin/llama-server
MODEL_PATH=/root/models/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf
LOG_FILE=/var/log/llama.log

# Check if binary exists
if [ ! -x "$LLAMA_BIN" ]; then
    echo "Error: llama-server not found at $LLAMA_BIN"
    exit 1
fi

# Check if model exists
if [ ! -f "$MODEL_PATH" ]; then
    echo "Error: Model not found at $MODEL_PATH"
    exit 1
fi

# Start server in background
echo "Starting llama-server..."
nohup $LLAMA_BIN \
    -m $MODEL_PATH \
    --port 8080 \
    --host 0.0.0.0 \
    -c 2048 \
    -t 4 \
    -ngl 0 \
    > $LOG_FILE 2>&1 &

echo "llama-server started (PID: $!)"
echo "Logs: $LOG_FILE"
