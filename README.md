# EDA_TP5

modificaciones realizadas sobre mkindex.cpp 

Paso 1: parsear la ruta de la carpeta
Lee el argumento -h desde la línea de comandos y verifica que sea una carpeta válida y existente. Esto define dónde buscar los archivos HTML.
 
1.1)creamos un objeto de CommandLineParser, parser, para leer los argumentos de linea de comandos (es decir, lo que esta luego del nombre del programa:
en este contexto dicho argumento seria la ruta de la carpeta www).
1.2) hasOption verifica si el argumento -h está presente. Retorna true si existe, false si no.
1.3)getOption obtiene el valor asociado con -h como un string.
1.4)exists de <filesystem>, verifica si la ruta wwwPath existe en el sistema de archivos.
1.5)is_directory confirma que wwwPath es una carpeta.


Paso 2: abrir la base de datos SQLite (ya estaba hecho)
 Abre (o crea) la base de datos index.db para almacenar el índice de búsqueda.

Paso 3: Crear tabla virtual con FTS5
Crea una tabla virtual search_index usando el módulo FTS5 de SQLite, con columnas path (ruta del archivo) y content (texto limpio), optimizada para búsquedas de texto completo.

  En esta instancia, primero habiamos optado por crear tres tablas: documents (con un ID unico para cada archivo y un path),
words ( con ID unico para cada palabra y la palabra con UNIQUE para evitar duplicados) y word_document: con word_id ( para relacionar las palabras con documentos, usando IDs numericos: mas eficientes).
Luego, investigando por mejores opciones, encontramos una extensión de SQLite: FTS5, diseñada para buscar texto de manera rápida y eficiente. En lugar de almacenar datos como tablas normales (como hicimos antes), FTS5 crea tablas virtuales que indexan palabras (tokens) automáticamente
Concluimos que, por un lado, el codigo anterior requiere múltiples tablas, consultas más complejas (para buscar una palabra, uniamos (JOIN) las tablas words y word_document, lo que podria ser más lento con muchos datos) y mantenimiento manual (teniamos que insertar palabras únicas en words y conectarlas con documentos en word_document). Por otro lado, con FTS5 se requiere una sola tabla virtual,tiene un índice automático que mapea palabras a documentos y , finalmente, requiere de mucho menos codigo.


Procesar archivos HTML
Qué hace: Construye la ruta a la carpeta www/wiki y verifica que exista, preparando el programa para recorrer los archivos HTML en el paso siguiente.

5.1. Listar los archivos: Encontrar todos los archivos .html en www/wiki 
5.2Leer el contenido: Abrir cada archivo y obtener su texto (que incluye etiquetas HTML como <p>, <h1>, etc.).
5.3Quitar etiquetas HTML: Eliminar manualmente todo lo que esté entre < y > para quedarte con el texto puro.
5.4Extraer palabras: Dividir el texto limpio en palabras individuales y prepararlas para guardarlas (en minúsculas, sin puntuación).

Paso 5: Indexar archivos en la base de datos
Qué hace: Recorre los archivos .html en www/wiki, lee su contenido, lo limpia con extractCleanText, y guarda la ruta relativa y el texto limpio en la tabla search_index usando transacciones.


funcion extractWordsFromText:Itera sobre el contenido del archivo carácter por carácter.
Usa inTag para ignorar texto entre < y > (etiquetas HTML).
Guarda el texto limpio en cleanText.
Divide cleanText en palabras usando stringstream.
Para cada palabra:
La convierte a minúsculas (transform con tolower).
Elimina puntuación (remove_if con ispunct).
Ignora palabras cortas (< 3 caracteres).
Las guarda en un set<string> para evitar duplicados.





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
