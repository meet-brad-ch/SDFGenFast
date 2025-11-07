"""
Pytest configuration for sdfgen tests

This file configures the Python import path so that tests can find
the sdfgen package created by the build process.
"""
import sys
from pathlib import Path

# Add project root to Python path so 'sdfgen' package can be imported
# The build process creates sdfgen/ at the project root
project_root = Path(__file__).parent.parent.parent
sys.path.insert(0, str(project_root))
