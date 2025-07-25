#!/bin/bash

echo "🚀 SGBD Distribuido - Inicio Rápido"
echo "==================================="

# Compilar si es necesario
if [ ! -f "bin/sgbd_distributed" ] || [ ! -f "bin/sgbd_main" ]; then
    echo "📦 Compilando sistema..."
    make all
fi

echo ""
echo "Opciones disponibles:"
echo "1. Sistema principal integrado (make run-main)"
echo "2. Sistema distribuido (make run-distributed)"  
echo "3. Sistema interactivo standalone (make run-interactive)"
echo ""
echo "Selecciona opción (1-3): "
read -r choice

case $choice in
    1)
        echo "🏢 Ejecutando sistema principal..."
        make run-main
        ;;
    2)
        echo "🌐 Ejecutando sistema distribuido..."
        make run-distributed
        ;;
    3)
        echo "💻 Ejecutando sistema interactivo..."
        make run-interactive
        ;;
    *)
        echo "❌ Opción inválida"
        ;;
esac
