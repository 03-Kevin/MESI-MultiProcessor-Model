# MESI-MultiProcessor-Model

**Simulación académica de un sistema multiprocesador con coherencia de caché MESI**  
Modelo orientado a la enseñanza: 4 Processing Elements (PEs) con cachés privadas 2-way, memoria compartida y un interconnect que implementa el protocolo MESI. Incluye ejemplos para ejecutar el benchmark del producto punto en paralelo y recopilar métricas.

---

## Estado del proyecto
**Avance:** Semana 1 — Planificación y definición (documentos y estructura inicial).  
**Objetivo actual:** Entregar Avance 1 con objetivos específicos, roles, extracción de requisitos y bibliografía.

---

## Descripción corta
Simulación académica de un sistema multiprocesador con coherencia MESI (4 PEs, cachés 2-way, write-back / write-allocate).

---

## Características principales
- 4 PEs, cada uno con 8 registros de 64 bits.  
- Caché privada por PE: 2-way set associative, 16 bloques × 32 bytes, write-allocate + write-back.  
- Protocolo MESI: estados y mensajes (BusRd, BusRdX, BusUpgr, Writeback...).  
- Memoria principal simulada: 512 posiciones de 64 bits.  
- Modelo hardware-like: componentes modelados con threads.  
- Recolección de métricas por PE: cache misses, invalidaciones, tráfico de bus, transiciones MESI, accesos R/W.  
- Benchmark: producto punto paralelo y validación automática.

---

## Requisitos 
- **Lenguaje recomendado:** C++ (recomendado) o SystemVerilog (no sintetizable).  
- **Herramientas sugeridas (C++):** CMake, g++/clang++.  
- **Sistema operativo:** Linux / macOS / Windows (con entorno de desarrollo).  
> Nota: Python **no** está permitido según la especificación del curso.

---

## Estructura
```
/docs/                 # Documentación (avance1.pdf, acta, bibliografía)
/diagrams/             # Diagramas (arquitectura.png)
/src/                  # Código fuente (simulador, módulos, utilidades)
/src/examples/         # Ejemplos ASM/benchmarks (producto punto por segmento)
/tests/                # Scripts de prueba y validación
/scripts/              # Scripts de build / run / análisis
/README.md
/LICENSE
```

---

## Cómo compilar / ejecutar 
> Ajustar comandos reales según la implementación.

### Ejemplo (C++ con CMake)
```bash
# clonar repo
git clone <repo-url>
cd MESI-MultiProcessor-Model

# compilar
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# ejecutar (placeholder)
./mp_mesi_simulator --load ../src/examples/pdot_seg.asm --mem-config ../config/mem.cfg
```

### Ejemplo (SystemVerilog - ModelSim/Questa)
```bash
vlog src/*.sv
vsim top_tb
```

---

## Archivos clave a incluir en la entrega Avance1
- `docs/avance1.pdf` — planificación, objetivos SMART, roles, requisitos técnicos extraídos (obligatorio).  
- `diagrams/arquitectura.png` — diagrama de alto nivel (PEs, cachés, interconnect, memoria).  
- `docs/cache_mesi.md` — especificación de caché y tabla de transiciones MESI.  
- `src/examples/pdot_seg.asm` — ejemplo ASM para un segmento del producto punto.  
- `docs/acta_reunion_inicial.pdf` — acta firmada por los 3 integrantes.  
- `README.md` — este archivo (actualizar con instrucciones reales).  

---

## Checklist Avance1 (pegar en el ZIP de entrega)
- [ ] `avance1.pdf` (portada, objetivos, requisitos, roles, plan, bibliografía).  
- [ ] `diagrams/arquitectura.png`.  
- [ ] `docs/cache_mesi.md`.  
- [ ] `src/examples/pdot_seg.asm`.  
- [ ] `docs/acta_reunion_inicial.pdf` (firmada).  
- [ ] `README.md` actualizado.  
- [ ] Primer commit: `init: estructura proyecto - avance1` y tag `avance1`.  
- [ ] ZIP con todo y subida a Tec Digital antes de la fecha.

---

## Convenciones de desarrollo
- Rama principal: `main` (siempre estable).  
- Ramas de trabajo: `feature/<nombre>` (una tarea por branch).  
- Mensajes de commit claros: `feat:`, `fix:`, `docs:`, `chore:`.  
- Abrir PRs para merges y usar reviewers entre los integrantes.

---

## Cómo contribuir
1. Fork → Clone → Crear branch: `feature/<nombre>`.  
2. Hacer commits atómicos y descriptivos.  
3. Abrir PR hacia `main` con descripción y pruebas realizadas.  
4. Asignar reviewer y aprobar antes de merge.

---