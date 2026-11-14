# Especificación KBOOM-1
## Preámbulo
### ¿Qué es?

_Nota: KBOOM-1 está diseñado para usarse en plataformas x86-64, por lo que todo el documento asumirá una arquitectura x86-64 con al menos un AMD Opteron como CPU. Sin embargo, teóricamente podría extenderse a otras arquitecturas._

KBOOM-1 _(Kernel BOOtstrap Metadata Version 1.X)_ es una especificación de arranque de kernel diseñada para servir como una capa intermedia entre el gestor de arranque (bootloader) y el kernel, de modo que este último se encuentre en un entorno de 64 bits predecible y determinista.

Está pensado especialmente (aunque no exclusivamente) para facilitar el proceso de carga de un kernel de 64 bits en la mitad superior (higher half) con énfasis en sistemas BIOS, aunque también es compatible con UEFI.

### Qué **NO** es

KBOOM-1 no es:
- Un gestor de arranque (bootloader)
- Un kernel
- Un firmware

### Desambiguación terminológica
- "**DEBE**" (MUST): Condición estricta. De no cumplirse, puede que arranque o no, dependiendo de la implementación.

- "**DEBERÍA**" (SHOULD): Condición recomendada. Se aconseja su cumplimiento, pero se garantiza que el kernel arrancará aunque no se siga. Sin embargo, la información podría no mantener su completa integridad.

## Estructuras de KBOOM-1

### Cabecera principal

La especificación KBOOM-1 cuenta con una cabecera a través de la cual se intercambia información desde el kernel hacia el cargador (bootstrapper) y _viceversa_. La cabecera **DEBE** ubicarse en el byte cero del binario plano (flat binary) del kernel y **DEBE** comenzar con una firma mágica de 8 bytes con la cadena "\_KBOOM1\_" (sin terminador nulo) como valor.

La disposición de la cabecera es la siguiente:
```
| Desplazamiento (bytes) | Nombre           | Tipo (estilo C)
|------------------------|--------------------------------------------------|
| 0x00                   | magic            | char[8]                       |
| 0x08                   | virt_base        | uint64_t                      |
| 0x10                   | entry            | uint64_t                      |
| 0x18                   | size             | uint64_t                      |
| 0x20                   | bss_base         | uint64_t                      |
| 0x28                   | bss_end          | uint64_t                      |
| 0x30                   | video_info       | kboom1_video_info*            |
| 0x38                   | region_table     | kboom1_memory_region*         |
| 0x40                   | region_table_sz  | uint64_t                      |
|------------------------|------------------|-------------------------------|
```

**Donde:**

- ```magic```: El valor mágico de identificación (```{'_','K','B','O','O','M','1','_'}```).
- ```virt_base```: La dirección virtual base a la que está enlazado el kernel.
- ```entry```: La dirección virtual del punto de entrada (entry point).
- ```size```: El tamaño del archivo en disco.
- ```bss_base```: La dirección virtual del inicio de la sección ```.bss```.
- ```bss_end```: La dirección virtual del final de la sección ```.bss```.
- ```video_info```: La dirección virtual de la estructura de información de vídeo donde el kernel espera que se coloque dicha información (```NULL``` para "no se espera información de vídeo").
- ```region_table```: La dirección virtual de la tabla de estructuras de regiones de memoria donde el kernel espera que se coloque el mapa de regiones (```NULL``` para "no se esperan regiones de memoria").
- ```region_table_sz```: El número máximo de entradas que puede contener la tabla de regiones. Si es cero, toma el valor por defecto de 128.

### Región de memoria (entrada, no la tabla. La tabla es simplemente un array)
```
| Desplazamiento (bytes) | Nombre           | Tipo (estilo C)
|------------------------|--------------------------------------------------|
| 0x00                   | type             | uint32_t                      |
| 0x04                   | base             | uint64_t                      |
| 0x0C                   | size             | uint64_t                      |
| 0x14                   | next_idx         | uint16_t                      |
| 0x16                   | used             | uint16_t                      |
| 0x18                   | flags            | uint64_t                      |
|------------------------|------------------|-------------------------------|
```

**Donde:**

- ```type```: El tipo de región según [E820](https://wiki.osdev.org/Detecting_Memory_(x86)) de la BIOS. El tipo 0 indica "Entrada libre".
- ```base```: La dirección física donde comienza la región.
- ```size```: El tamaño de la región en bytes.
- ```next_idx```: El índice del siguiente elemento. El valor terminador es ~0 (0xFFFF).
- ```used```: Para almacenar metadatos del PMA del cargador (puede variar entre implementaciones; no es fiable). Debe ser ignorado por el kernel o utilizado por su propio PMA tras limpiar el campo.
- ```flags```: Las banderas (flags) de la región. Aún no hay banderas definidas.

### Información de vídeo
```
| Desplazamiento (bytes) | Nombre           | Tipo (estilo C)
|------------------------|--------------------------------------------------|
| 0x00                   | fbaddr           | uint64_t                      |
| 0x08                   | fbwid            | uint32_t                      |
| 0x0C                   | fbhei            | uint32_t                      |
| 0x10                   | fbpitch          | uint32_t                      |
| 0x14                   | fbbpp            | uint32_t                      |
|------------------------|------------------|-------------------------------|
```

**Donde:**

- ```fbaddr```: La dirección física del framebuffer.
- ```fbwid```: El ancho del framebuffer en píxeles.
- ```fbhei```: El alto del framebuffer en píxeles.
- ```fbpitch```: El ancho de una línea de escaneo (scanline) en bytes.
- ```fbbpp```: Los bits por píxel del framebuffer.

## Contrato ABI de KBOOM-1
### Alias
Para facilitar la comprensión, se utilizarán los siguientes alias para acortar ciertos nombres:

- ```m_hdr``` = Cabecera principal (Main header).
- ```memtable``` = Tabla de estructuras de regiones de memoria.
- ```vinfo``` = Estructura de información de vídeo.

### Precondiciones

- ```m_hdr.magic``` **DEBE** tener el valor mágico de KBOOM-1 (```{'_','K','B','O','O','M','1','_'}```).

- ```m_hdr.virt_base``` **DEBE** estar alineada a 2 MB. Sin embargo, el cargador puede cargarla de todos modos, truncándola para alinearla.

- ```m_hdr.bss_end``` **DEBE** ser mayor o igual que ```m_hdr.bss_base```.

- ```m_hdr.bss_base``` **DEBE** ser mayor o igual que ```m_hdr.virt_base```.

- ```m_hdr.bss_base``` **DEBERÍA** ser mayor o igual que ```m_hdr.virt_base``` + ```m_hdr.size```.

- ```m_hdr.entry``` **DEBE** pertenecer al intervalo [```m_hdr.virt_base```, ```m_hdr.virt_base``` + ```m_hdr.size```).

- ```m_hdr.video_info``` **DEBE** pertenecer al intervalo [```m_hdr.virt_base```, ```m_hdr.bss_end```).

- ```m_hdr.region_table``` **DEBE** pertenecer al intervalo [```m_hdr.virt_base```, ```m_hdr.bss_end```).

- ```m_hdr``` **DEBE** colocarse al principio mismo del archivo binario del kernel (desplazamiento 0).

- El kernel **DEBE** proporcionarse como un archivo binario plano (flat binary).

### Garantías

Si se cumplen las condiciones anteriores y la implementación del cargador no está dañada, se garantiza que las siguientes afirmaciones sobre el estado de la máquina son verdaderas:

#### Registros
- ```CS``` = ```0x08```
- ```DS, SS, etc.``` = ```0x10```
- ```FS, GS``` = ```0x00```
- ```RIP``` = ```m_hdr.entry```
- ```CR3```: Estructura PML4 válida, pero fiable solo a corto plazo.
- ```RSP```: Pila (stack) válida, pero fiable solo a corto plazo.

_Nota: RSP y CR3 no son fiables a largo plazo porque las estructuras pueden ser sobrescritas por el kernel si no se rastrean._

#### Tablas de control
- ```IDT```: Nula o no fiable.
- ```LDT```: Nula o no fiable.
- ```GDT```: Válida, pero fiable solo a corto plazo.

_Nota: La GDT no es fiable a largo plazo porque la estructura puede ser sobrescrita por el kernel si no se rastrea._

#### Otros
- ```.bss```: Inicializada a ceros.
- ```IF``` = ```0```

**Ninguna otra estructura, registro o bandera tiene un estado garantizado al entrar.**

### Mapeo
La paginación inicial mapeará, como mínimo, los siguientes rangos: 
- ```m_hdr.virt_base``` -> ```m_hdr.bss_end```
- ```0x0000000000000000``` -> ```0x00000000FFFFFFFF```
- ```vinfo.fbaddr``` -> ```vinfo.fbaddr + vinfo.pitch * vinfo.fbhei```

## Deberes del kernel
Al principio de su ejecución, el kernel **DEBE** configurar su propia pila (stack), GDT y paginación, así como (opcionalmente) una IDT.
