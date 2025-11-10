# Serialization of Data & Data Structures in SOME/IP

---

## 🧠 What is Serialization (4.1.4)

**Serialization** means converting **structured data** into a **byte stream** suitable for transmission over a network.  
It defines **how fields are ordered, aligned, and padded** in the message.

### 🔹 Key Rules

- The **order and position** of every field are fixed by the `.arxml` service/interface definition.
    
- **Alignment** ensures that multi-byte data starts at aligned memory boundaries (2, 4, or 8 bytes).
    
- **Padding bytes** are added after variable-sized data (unless it’s the last field).
    
- Padding values are **undefined** — the receiver ignores them.
    
- The goal: **byte-perfect interoperability** between ECUs from different vendors.
    

---

## 🧩 Basic Datatypes (4.1.4.1)

All SOME/IP messages are built from **primitive fixed types**:

|Type|Size|Notes|
|---|---|---|
|`boolean`|1 byte|Only bit 0 used (0 = FALSE, 1 = TRUE); bits 1–7 ignored.|
|`uint8/16/32/64`|8–64 bits|Unsigned integer.|
|`sint8/16/32/64`|8–64 bits|Signed integer in two’s complement.|
|`float32`|4 bytes|IEEE 754 single precision.|
|`float64`|8 bytes|IEEE 754 double precision.|

**Byte order (endianness)**: configurable per field (usually **big-endian / network order**).  
Serialization rules are standardized to ensure consistent interpretation by all ECUs.

---

## 🧱 Structured Datatypes (Structs) (4.1.4.2)

A **struct** groups multiple basic types into one logical unit — similar to C structs.

### 🔹 Serialization Rules

- Fields are serialized **sequentially**, as defined in the ARXML interface.
    
- **No hidden padding** is automatically added.
    
- An **optional length field** (8, 16, or 32 bits) may precede the struct:
    
    - Indicates total struct size in bytes.
        
    - If longer → extra bytes ignored.
        
    - If shorter → message considered malformed.
        

### 🔹 Nested Structs (Depth-First Serialization)

- Inner structs are serialized **before** outer members.
    
- Flattened inline in the final byte stream.
    

#### Example:

```c
struct A {
  uint16 id;
  struct B {
     uint32 time;
     uint8 status;
  } nested;
  float64 value;
};
```

**Serialization order:**

```
id → time → status → value
```

**Depth-first traversal:** inner members first, no extra bytes.

---

## 🏷️ Tagged Serialization (4.1.4.3)

Tagged serialization adds **Data IDs (tags)** to fields, allowing **forward/backward compatibility**.

### 💡 Concept

- Makes data **self-descriptive** using **TLV (Tag-Length-Value)** format.
    
- Supports optional or evolving fields.
    
- Rarely used directly — handled by SOME/IP stack (e.g., `vsomeip`).
    

**Current relevance:**  
Low — only conceptual understanding needed unless designing custom serializers.

---

## 🧵 Dynamic Length Strings (4.1.4.4.2)

Used for **variable-length text data** (names, messages, file paths, etc.).

### 🔹 Layout

```
[Length field] [BOM (optional)] [Characters] ['\0']
```

- `Length` = number of bytes in the string (includes BOM + '\0', but **excludes** length field itself).
    
- `Length` can be 8, 16, or 32 bits (default = 32).
    
- String **must** end with `'\0'`.
    
- Measured in **bytes**, not characters.
    
- **BOM** (Byte Order Mark) is optional but counted in length.
    

#### Example:

Text = `"Hi"`  
Length = 3 → includes `'H'`, `'i'`, and `'\0'`  
Serialized:

```
00 00 00 03 48 69 00
```

### 🔹 Legacy Mode

- Strings can also be plain byte arrays (no BOM, no `\0`).
    
- Enabled by `implementsLegacyStringSerialization = true`.
    
- Common in low-resource embedded ECUs.
    

### 🔹 Why It Matters

- Prevents overflow and ambiguity in mixed messages.
    
- Critical for OTA, file paths, and configuration data exchange between ECUs.
    

---

## 🧮 Fixed-Length Arrays (4.1.4.5.1)

Represent multiple elements of the **same type with known size (n)**.

### 🔹 Layout

```
[Optional LengthField] [Element1] [Element2] ... [ElementN]
```

**Total size** = LengthField + (n × element_size)

### 🔹 Rules

- Optional length field (8/16/32 bits):
    
    - Allows overrun/underrun detection.
        
    - If actual length > expected → extra bytes ignored.
        
    - If shorter → malformed.
        
- Without length field → size inferred from type definition.
    
- **One-dimensional arrays:** sequential layout.
    
- **Multidimensional arrays:** row-major order; each row may have its own length field.
    

#### Example:

```c
uint8 data[3] = {10, 20, 30};
```

Serialized:

```
[Length=03] [0A 14 1E]
```

→ Total = 4 bytes.

**Use cases:** sensor data, CAN frame snapshots, fixed signals.

---

## ♻️ Dynamic Arrays (4.1.4.5.2)

Used when the number of elements **varies dynamically** (e.g., file chunks, OTA updates).

### 🔹 Layout

```
[Length field] [Element1] [Element2] ... [ElementN]
```

- `Length` = total size in bytes (excluding the length field itself).
    
- Default `Length` field size = 32 bits (can be 0, 8, 16, or 32).
    
- Each nested array has its own length field.
    

### 🔹 Rules

- Same serialization logic as fixed arrays.
    
- If `Length` > expected → extra skipped.
    
- If `Length` < expected → message malformed unless recoverable.
    
- For fixed-size elements:  
    `Number of items = Length / element_size`.
    

#### Example:

```c
uint8 data[] = {A1, B2, C3};
```

Serialized:

```
00 00 00 03 A1 B2 C3
```

→ Dynamic array of 3 bytes.

### 🔹 Multidimensional Example

```
OuterLength
 ├─ Row1_Length + Row1_Elements
 ├─ Row2_Length + Row2_Elements
 ...
```

Each row can have a different number of elements.

**Use Case:**  
Transferring OTA data blocks or variable sensor data packets.

---

## 🧾 Enumeration (4.1.4.6)

Enumerations are transmitted as **plain unsigned integers**.  
There’s **no special encoding** for enums in SOME/IP.

### 🔹 Rules

- Sent as numeric constants (e.g., `uint8`, `uint16`).
    
- Meaning is defined in `.arxml` data type definition.
    

#### Example:

```c
enum Mode { OFF = 0, ON = 1, ERROR = 2 };
```

Transmitted:

```
00 / 01 / 02  → uint8
```

**Note:** `[PRS_SOMEIP_00705]` states that “Enumerations are not considered in SOME/IP.”  
Interpretation is done purely by the application.

---

## ⚙️ Bitfield (4.1.4.7)

Bitfields pack multiple boolean flags into a single integer — reducing bandwidth.

### 🔹 Rules

- Transported as `uint8`, `uint16`, or `uint32`.
    
- Each bit has a defined meaning.
    
- The entire byte/word is sent as one unsigned integer — not as individual bits.
    

#### Example:

```
bit7...bit0 = [0 | 0 | 0 | 0 | 0 | CRC | ERR | READY]
READY=1, ERR=0, CRC=1 → 00000101 → 0x05
```

Transmitted: `uint8(0x05)`

Receiver uses bit masks to interpret:

```c
READY = value & 0x01;
ERR   = value & 0x02;
CRC   = value & 0x04;
```

**Advantage:** Compact and efficient boolean transmission.

---

## 🧮 Union / Variant (4.1.4.8)

A **Union (Variant)** allows one field to hold **different datatypes at runtime**.

### 🔹 Layout

```
[Length field] [Type field] [Data (+optional Padding)]
```

|Field|Description|
|---|---|
|Length|Size of Data + Padding (excludes Length & Type fields).|
|Type|Identifies which datatype follows (8/16/32 bits).|
|Data|Actual payload according to Type.|
|Padding|Optional for alignment.|

### 🔹 Rules

- `Length` can be 0, 8, 16, or 32 bits (default = 32).
    
    - If `0` → all contained types must have same size.
        
- `Type = 0` → represents **NULL** (no data).
    
- Each type in the union has a unique type code.
    
- If lengths mismatch → receiver skips or flags error.
    

#### Example:

```text
Union { uint8, uint16 }

Send uint8(0x7A):
Length=1, Type=1, Data=7A

Send uint16(0x1234):
Length=2, Type=2, Data=12 34
```

### 🔹 Behavior

- Receiver reads `Type`, interprets `Data` accordingly.
    
- Padding may be added for alignment.
    
- Enables flexible messages with runtime-selected formats.
    

### 🔹 Use Case

- Diagnostic or telemetry systems with variable payloads.
    
- OTA or logging systems: different message types (progress, error, chunk info).
    

**Summary:**

|Field|Purpose|
|---|---|
|Length|Total data size|
|Type|Identifies inner datatype|
|Data|Serialized content|
|0 Type|NULL value allowed|

---

## ✅ Final Summary Table

|Data Type|Length Field|Variable Size|Notes / Use Case|
|---|---|---|---|
|**Basic Types**|❌|❌|Fixed byte size (boolean, uint, float)|
|**Structs**|Optional|❌|Grouped basic types, depth-first|
|**Tagged Structs**|✅|✅|Forward/backward compatibility|
|**String**|✅|✅|Includes '\0'; may have BOM|
|**Fixed Array**|Optional|❌|Predictable layout, simple|
|**Dynamic Array**|✅|✅|For variable-length data (e.g., OTA)|
|**Enum**|❌|❌|Plain numeric value|
|**Bitfield**|❌|❌|Compact flags in one word|
|**Union/Variant**|✅|✅|Runtime-selectable type container|

---

### 🧩 Key Takeaways

- **Serialization defines the on-wire structure** of all SOME/IP messages.
    
- **Alignment and length fields** ensure compatibility and parsing reliability.
    
- **Dynamic types** (arrays, strings, unions) allow flexibility for OTA and telemetry.
    
- **Static types** (structs, enums, bitfields) ensure deterministic communication.
    
- Middleware (like `vsomeip`) automates this, but understanding the format is crucial for debugging or defining `.arxml` interfaces manually.
    

---