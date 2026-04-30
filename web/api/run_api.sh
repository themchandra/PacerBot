#!/bin/bash
# Quick startup script for PacerBot API

set -e

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🤖 PacerBot Control API${NC}"
echo "Starting API server..."

# Check if virtual environment exists
if [ ! -d "venv" ]; then
    echo "Creating virtual environment..."
    python3 -m venv venv
fi

# Activate venv
source venv/bin/activate

# Install/update dependencies
echo "Installing dependencies..."
pip install -q -r requirements.txt

echo -e "${GREEN}✓ Ready to start!${NC}"
echo ""
echo "Starting FastAPI server..."
echo "📋 API Docs: http://localhost:8000/docs"
echo "🏥 Health:  http://localhost:8000/health"
echo ""

# Start the API
python main.py
