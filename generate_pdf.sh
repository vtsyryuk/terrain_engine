#!/bin/bash

# Convert Markdown documentation to PDF
# Requirements: pandoc, pdflatex (or other PDF engine)
# Install: brew install pandoc basictex

if ! command -v pandoc &> /dev/null; then
    echo "ERROR: pandoc is not installed"
    echo "Install: brew install pandoc"
    exit 1
fi

cd "$(dirname "$0")"

echo "Converting DOCUMENTATION.md to PDF..."

pandoc DOCUMENTATION.md \
    -o DOCUMENTATION.pdf \
    -V lang=ru-RU \
    -V geometry:margin=2cm \
    -V fontfamily=noto \
    --toc \
    --toc-depth=2 \
    --from markdown \
    --to pdf \
    --pdf-engine=pdflatex

if [ $? -eq 0 ]; then
    echo "✓ PDF created: DOCUMENTATION.pdf"
    ls -lh DOCUMENTATION.pdf
else
    echo "ERROR: Conversion failed"
    exit 1
fi
