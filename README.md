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
Qué es FTS5
SQLite FTS5 (Full-Text Search, versión 5) es una extensión de SQLite diseñada para buscar texto de manera rápida y eficiente. En lugar de almacenar datos como tablas normales, FTS5 crea tablas virtuales que indexan palabras (tokens) automáticamente, lo que permite búsquedas como las de un motor de búsqueda (por ejemplo, buscar "hola" en un conjunto de documentos).

Por qué es más eficiente
El código actual con tres tablas (documents, words, word_document) funciona bien, pero:

Requiere múltiples tablas: Gestionás tres tablas y sus relaciones (claves foráneas, índices).
Consultas más complejas: Para buscar una palabra, necesitás unir (JOIN) las tablas words y word_document, lo que puede ser más lento con muchos datos.
Mantenimiento manual: Vos mismo tenés que insertar palabras únicas en words y conectarlas con documentos en word_document.
FTS5 es más eficiente porque:

Una sola tabla virtual: Reemplaza las tres tablas con una tabla optimizada para búsqueda.
Índice invertido: FTS5 crea un índice automático que mapea palabras a documentos, haciendo las búsquedas más rápidas (O(log n) o mejor en la mayoría de los casos).
Soporte avanzado: Permite buscar frases, prefijos (como Java*), y combinaciones (hola AND mundo).
Menos código: No necesitás gestionar claves foráneas ni índices manuales; FTS5 lo hace por vos.


Por qué es más eficiente
El código actual con tres tablas (documents, words, word_document) funciona bien, pero:

Requiere múltiples tablas: Gestionás tres tablas y sus relaciones (claves foráneas, índices).
Consultas más complejas: Para buscar una palabra, necesitás unir (JOIN) las tablas words y word_document, lo que puede ser más lento con muchos datos.
Mantenimiento manual: Vos mismo tenés que insertar palabras únicas en words y conectarlas con documentos en word_document.
Qué es: Una transacción en una base de datos es como un "paquete" de operaciones que deben hacerse todas juntas o ninguna. Es una forma de asegurarte de que los cambios que hacés en la base de datos (como guardar un archivo) se completen correctamente, o si algo falla, todo se deshace para evitar errores.
En el código: Usamos transacciones para guardar información de cada archivo HTML (su ruta y texto) en la base de datos. Si algo falla (por ejemplo, no se puede guardar el texto), la transacción se "deshace" (rollback) para no dejar datos incompletos.
Palabras clave:
BEGIN;: Empieza la transacción (como decir "empecemos a armar el rompecabezas").
COMMIT;: Confirma los cambios (como decir "terminé, todo encaja, guardemos el rompecabezas").
ROLLBACK;: Deshace los cambios si algo falla (como sacar todas las piezas si no encajan).
Qué es una consulta SQL: SQL es un lenguaje para hablar con bases de datos. Una consulta SQL es como una instrucción que le das a la base de datos, por ejemplo, "guarda este texto" o "busca esta palabra".
Qué es "compilar una consulta SQL": Compilar una consulta significa tomar una instrucción SQL (como INSERT INTO tabla (columna) VALUES (?);) y prepararla para que la base de datos la entienda y la ejecute rápido varias veces. Es como traducir una receta escrita a un formato que la cocina puede usar una y otra vez sin tener que releer la receta completa.
Qué es una declaración preparada: Una declaración preparada es como una plantilla de una instrucción SQL que tiene "huecos" (marcados con ?) donde podés poner diferentes valores cada vez que la uses. Por ejemplo:
Plantilla: INSERT INTO search_index (path, content) VALUES (?, ?);
Uso: Llenás los ? con valores como "wiki/doc1.html" y "hola mundo".
Qué hace sqlite3_prepare_v2:
Toma una consulta SQL (la plantilla) y la convierte en un objeto que SQLite puede ejecutar muchas veces.
Es más seguro y rápido que ejecutar consultas directamente, especialmente si vas a repetir la misma operación (como guardar muchos archivos).





 cambios:
 std::string extractCleanText(const std::string& content) {
    std::string cleanText;
    bool inTag = false;

    // Remover etiquetas HTML
    for (char c : content) {
        if (c == '<') inTag = true;
        else if (c == '>') inTag = false;
        else if (!inTag) cleanText += tolower(c);
    }

    // Remover puntuación y normalizar espacios
    std::string result;
    bool lastWasSpace = true;
    for (char c : cleanText) {
        if (ispunct(c)) continue;
        if (isspace(c)) {
            if (!lastWasSpace) result += ' ';
            lastWasSpace = true;
        } else {
            result += c;
            lastWasSpace = false;
        }
    }
    // Remover espacio final
    if (!result.empty() && result.back() == ' ') result.pop_back();
    return result;
}Qué hace:

Quita etiquetas HTML (como <p> o </div>) usando un bucle que detecta < y >.
Convierte el texto a minúsculas con tolower.
Elimina puntuación con ispunct y normaliza espacios (evita múltiples espacios seguidos).
Devuelve una cadena limpia (por ejemplo, <p>Hola, mundo!</p> se convierte en "hola mundo").
Ventajas del enfoque actual:

Simple de entender: Recorre el texto carácter por carácter, lo que es claro si estás empezando.
Control total: Vos decidís exactamente qué caracteres quitar o mantener.
Eficiente: Para textos pequeños o medianos, este bucle es rápido porque no usa bibliotecas adicionales.
Desventajas:

Propenso a errores: No maneja casos complejos de HTML (como <script>, comentarios <!-- -->, o atributos como <p class="test">).
Código largo: Requiere varios bucles y condiciones para tareas comunes (quitar etiquetas, normalizar espacios).
Mantenimiento difícil: Si querés cambiar cómo se limpian las palabras (por ejemplo, aceptar guiones), tenés que modificar el bucle manualmente.

Ventajas de regex:

Código más corto: Logra lo mismo con menos líneas, ya que regex reemplaza bucles complejos.
Flexible: Podés cambiar los patrones fácilmente para manejar casos nuevos (por ejemplo, aceptar números con [a-z0-9]).
Robusto: Maneja casos complejos de HTML (como <br/> o <!-- comentarios -->) con patrones bien definidos.
Estándar: Regex es un estándar en muchos lenguajes, por lo que aprendés una habilidad reutilizable.






##BUSCADOR
En la parte del buscador, lo primero que se hace es tokenizar la búsqueda del usuario. esto es, se separa todo el string de búsqueda en palabras individuales. Si el usuario buscó "100 metros", guardaremos "100" y "metros". 

luego de esto, se hace un query a la base de datos creada por mkindex, llamada index.db. Lo que nuestro código busca, son coincidencias (al menos una vez) por cada palabra tokenizada. si se hizo la búsqueda con "100 metros", nuestro código de fondo buscará todos los documentos que tengan una coincidencia para "100" y otra para "metros". SQlite nos devolverá todos los paths de los archivos donde se cumplieron las condiciones, y nuestro frontend podrá presentarlos al usuario como hipervínculos que llevan al archivo. Estos paths se devuelven por medio de un vector, para contenerlos y poder trabajarlos todos juntos. 
Como el archivo está en formato HTML, se puede visualizar por medio del mismo navegador web del que se accede al servidor autohosteado. No solo tenemos nuestro propio google, sino que tenemos también nuestro propio visualizador de HTML y servidor con archivos. Tenemos casi nuestro propio internet! (a muy baja escala, muy limitado, y extremadamente básico, pero nuestro)

Algo que no fue implementado fue algún tipo de ordenamiento de resultados. De momento, el código devuelve todos los hits sin dar ningún orden o comportamiento definido. queda en manos de SQL y cómo decide devolver el vector de resultados. Como mejora a futuro, se podría implementar un algoritmo que dé mayor peso a palabras clave si están en el titulo del documento, o que ordene por repetición de palabras (esto es, si un documento tiene la palabra buscada 20 veces, probablemente sea más relevante que otro documento donde la palabra buscadda aparece una sola vez. por tanto, deberíamos presentarle al usuario primero los documentos con más hits.)
