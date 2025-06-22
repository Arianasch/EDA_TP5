# EDA_TP5

pasos en mkindex.cpp

1)
Agregue:
#include <filesystem>: Para recorrer la carpeta www/wiki.
#include <fstream>: Para leer los archivos HTML.
#include <sstream>: Para dividir el texto en palabras.
#include <algorithm>: Para convertir a minúsculas.

2) parsear ruta de la carpeta:
 
2.1)creamos un objeto de CommandLineParser, parser para leer los argumentos
de linea de comandos (es decir, lo que esta luego del nombre del programa:
en este contexto dicho argumento seria la ruta de la carpeta www).

3)abrir la base de datos SQLite (ya estaba hecho)

4) crear tablas
4.1 empezar y terminar transacciones:
if (sqlite3_exec(database, "BEGIN TRANSACTION;", NULL, 0, &databaseErrorMessage) != SQLITE_OK)
Qué hace: Inicia una transacción para agrupar todas las operaciones de creación de tablas.
Por qué: Mejora el rendimiento y asegura que, si algo falla, no queden tablas a medio crear.
Validación: Si falla (ej., problema de permisos), muestra el error, libera databaseErrorMessage, cierra la base de datos y termina.
if (sqlite3_exec(database, "DROP TABLE IF EXISTS word_document;", NULL, 0, &databaseErrorMessage) != SQLITE_OK)
Qué hace: Borra las tablas word_document, words, y documents si ya existen.
Por qué: Evita errores como "table already exists" si ejecutás el programa varias veces.
Orden: Primero word_document (por las claves foráneas), luego words y documents.
Validación: Si falla, muestra el error, libera recursos y termina.


4.2)crea tabla documents: con doc_id (ID unico para cada archivo)
patb (ruta relartiva del archivo)_
Por qué: Almacena cada archivo HTML una vez, con un ID para usarlo en relaciones.
Validación: NOT NULL asegura que path no esté vacío.

4.3 crea tabla words: con word_id (ID unico para cada palabra)
word (la palar=bra en si con UNIQUE oara evitar duplicados).
Por qué: Garantiza que cada palabra se guarde solo una vez, reutilizando su word_id.
Validación: UNIQUE evita repeticiones, como explicamos antes.

4.4) crea tabla word_document: con word_id (ID de la palabra, de words) y doc_id (ID DEL ARCHIVO, de documents)
Por qué: Relaciona palabras con documentos, permitiendo múltiples palabras por documento y viceversa.
PRIMARY KEY (word_id, doc_id): Evita duplicados de la misma relación.
FOREIGN KEY: Documenta la relación (aunque SQLite no las aplica por defecto).
Por qué: Conecta palabras con archivos de forma eficiente usando IDs numéricos.

4.5) crear indice :Qué hace: Crea un índice en words.word para acelerar búsquedas.
Por qué: Buscar palabras (ej., SELECT word_id FROM words WHERE word = 'evolucion';) será más rápido.

if (sqlite3_exec(database, "CREATE INDEX IF NOT EXISTS idx_word ON words(word);", NULL, 0, &databaseErrorMessage) != SQLITE_OK)

4.6 confirma transaccion:if (sqlite3_exec(database, "COMMIT;", NULL, 0, &databaseErrorMessage) != SQLITE_OK)

Qué hace: Guarda todos los cambios (creación de tablas e índice).
Por qué: Asegura que las tablas se creen correctamente o no se cree nada si hay un error.
Validación: Si falla, muestra el error y termina.

Mensajes de depuración:
cout << "Creando tablas..." << endl; y cout << "Tablas creadas con éxito" << endl; te ayudan a saber que el paso se ejecutó correctamente.


5) procesasr ois archivos HTML
5.1. Listar los archivos: Encontrar todos los archivos .html en www/wiki (ej., wiki/Evolucion_biologica.html).
5.2Leer el contenido: Abrir cada archivo y obtener su texto (que incluye etiquetas HTML como <p>, <h1>, etc.).
5.3Quitar etiquetas HTML: Eliminar manualmente todo lo que esté entre < y > para quedarte con el texto puro.
5.4Extraer palabras: Dividir el texto limpio en palabras individuales y prepararlas para guardarlas (en minúsculas, sin puntuación).


funcion extractWordsFromText:Itera sobre el contenido del archivo carácter por carácter.
Usa inTag para ignorar texto entre < y > (etiquetas HTML).
Guarda el texto limpio en cleanText.
Divide cleanText en palabras usando stringstream.
Para cada palabra:
La convierte a minúsculas (transform con tolower).
Elimina puntuación (remove_if con ispunct).
Ignora palabras cortas (< 3 caracteres).
Las guarda en un set<string> para evitar duplicados.


6)
