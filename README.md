# EDA_TP5  
INTEGRANTES: Sambucetti Juan, Schiaffino Ariana

RESUMEN DEL TRABAJO: El proyecto desarrolla un motor de búsqueda para archivos HTML almacenados en la carpeta www/wiki. mkindex.cpp recorre recursivamente esta carpeta, extrae el texto limpio de cada archivo HTML (eliminando etiquetas y puntuación con regex), calcula rutas relativas, y los indexa en una base de datos SQLite (index.db) usando una tabla virtual FTS5 con columnas path y content. HttpRequestHandler.cpp maneja solicitudes HTTP, consulta search_index con FTS5 para encontrar coincidencias, y genera una página HTML con enlaces a los archivos correspondientes.


***************************************************************************************************************************************************************
Modificaciones realizadas sobre mkindex.cpp (optamos por describir brevemente en pasos con el fin de que se mas claro para el lector y, a su vez, para mostrar 
como organizamos el codigo.


-Paso 1: parsear la ruta de la carpeta
Lee el argumento -h desde la línea de comandos y verifica que sea una carpeta válida y existente. Esto define dónde buscar los archivos HTML.
 
1.1)creamos un objeto de CommandLineParser, parser, para leer los argumentos de linea de comandos (es decir, lo que esta luego del nombre del programa:
en este contexto dicho argumento seria la ruta de la carpeta www).

1.2) hasOption verifica si el argumento -h está presente. Retorna true si existe, false si no.

1.3)getOption obtiene el valor asociado con -h como un string.

1.4)exists de <filesystem>, verifica si la ruta wwwPath existe en el sistema de archivos.

1.5)is_directory confirma que wwwPath es una carpeta.



-Paso 2: abrir la base de datos SQLite (ya estaba hecho)
 Abre (o crea) la base de datos index.db para almacenar el índice de búsqueda.



-Paso 3: Crear tabla virtual con FTS5
Crea una tabla virtual search_index usando el módulo FTS5 de SQLite, con columnas path (ruta del archivo) y content (texto limpio), optimizada para búsquedas de texto completo.

  En esta instancia, primero habiamos optado por crear tres tablas: documents (con un ID unico para cada archivo y un path),
words ( con ID unico para cada palabra y la palabra con UNIQUE para evitar duplicados) y word_document: con word_id ( para relacionar las palabras con documentos, usando IDs numericos: mas eficientes).
Luego, investigando por mejores opciones, encontramos una extensión de SQLite: FTS5, diseñada para buscar texto de manera rápida y eficiente. En lugar de almacenar datos como tablas normales (como hicimos antes), FTS5 crea tablas virtuales que indexan palabras (tokens) automáticamente
Concluimos que, por un lado, el codigo anterior requiere múltiples tablas, consultas más complejas (para buscar una palabra, uniamos (JOIN) las tablas words y word_document, lo que podria ser más lento con muchos datos) y mantenimiento manual (teniamos que insertar palabras únicas en words y conectarlas con documentos en word_document). Por otro lado, con FTS5 se requiere una sola tabla virtual,tiene un índice automático que mapea palabras a documentos y , finalmente, requiere de mucho menos codigo.

 En el contexto de FTS5, los tokens son las palabras que SQLite indexa para que puedas buscarlas después (FTS5 elimina acentos y convierte a minúsculas internamente para búsquedas, dependiendo del tokenizador).
 Al tokenizar, SQLite crea un índice interno que asocia cada token (palabra) con los documentos (filas en search_index) donde aparece. Esto hace que buscar una palabra sea rápido, porque SQLite ya sabe en qué filas está esa palabra.
Ademas, el tokenizador que usa FTS5 para dividir el texto en tokens es unicode61 el cual stá basado en las reglas de Unicode 6.1, que definen cómo identificar palabras en diferentes idiomas, incluyendo caracteres especiales como acentos, la ñ, o caracteres no latinos (ej., kanji).Convierte a minúsculas para búsquedas insensibles a mayúsculas.Ignora puntuación y espacios para enfocarse en palabras.



-Paso 4:Procesar archivos HTML
Construye la ruta a la carpeta www/wiki y verifica que exista, preparando el programa para recorrer los archivos HTML en el paso siguiente.
WikiPath es un string que almacena dicha ruta, para construirla se hace uso de la funcion path y se le pasa como argumento la variable con la ruta a la carpeta raiz obtenida del argumento -h (asi lo convierte en un objeto path de la bibilioteca filesystem: un objeto de dicha biblioteca es una herramienta para manejar rutas de forma segura y portátil). Luego, se usa el operador / de filesystem para concatenar el objeto path con la cadena "wiki", formando un nuevo objeto path que representa la ruta ../../www/wiki ("wiki" es el nombre de la subcarpeta dentro de www donde están los archivos HTML que queremos indexar). Finalmente, string() convierte el objeto path resultante de vuelta a un string para almacenarlo en la varible WikiPath.



-Paso 5: Indexar archivos en la base de datos
Este paso recorre todos los archivos HTML en la carpeta wikiPath, lee su contenido, lo limpia con extractCleanText, calcula su ruta relativa (para la columna path), y los inserta en la tabla search_index con las columnas path y content. Usa transacciones para asegurar que las inserciones sean consistentes.
Antes de comenzar, aclaramos conceptos como trasacciones, consultas y declaraciones preparadas en una base de datos.Luego, en el código, usamos transacciones para guardar información de cada archivo HTML (su ruta y texto) en la base de datos. Si algo falla (por ejemplo, no se puede guardar el texto), la transacción se "deshace" para no dejar datos incompletos.
Es importante mencionar que recorre recursivamente todos los archivos y subcarpetas en wikiPath donde recursive_directory_iterator de filesystem, crea un iterador que recorre todos los archivos y subcarpetas.Pero, por que? Los archivos HTML pueden estar organizados en subcarpetas dentro de wiki, un recorrido no recursivo  omitiría las subcarpetas.Asi, se permite indexar todos los archivos HTML, sin importar cuán profundo estén en la estructura de directorios.
Tambien, cuando se abre el archivo en modo lectura, se lee todo su contenido en un stringstream (buffer) y luego se convierte el contenido a un string (content).

Luego, se llama a extractCleanText para procesar el contenido HTML (content) y obtener texto limpio (sin etiquetas, scripts, puntuación, en minúsculas).
Dicha funcion hace uso de la libreria regex (elimina comentarios HTML,scripts, etiquetas HTML, puntuación ([^a-z\s]),Normaliza espacios y convierte el texto a minúsculas.
Ahora bien, antes habiamos implementado una funcion que lo hacia "a mano", sin usar esta libreria, por ejemplo, eliminaba etiquetas HTML usando un bucle que detectaba < y >. Si bien era simple de entender (recorre el texto carácter por carácter), era propenso a errores: no maneja casos complejos de HTML (como <script>, comentarios <!-- -->) y hacia el codigo largo: requiere varios bucles y condiciones para tareas comunes (quitar etiquetas, normalizar espacios).
Por otro lado, regex logra lo mismo con menos líneas,maneja casos complejos de HTML y , al ser un estándar en muchos lenguajes, encontramos de gran utlidad aprender a manejarlo.

Finalmnete, calcula la ruta relativa (relPath) respecto a wwwPath, e inserta relPath y cleanText en search_index dentro de una transacción.



-Paso 6: finalizar y cerrar la base de datos
 Libera la consulta preparada del Paso 5 y cierra la conexión a la base de datos (index.db), asegurando que todos los recursos se liberen correctamente y que los datos indexados queden guardados.
 Asi,finaliza el proceso de indexación limpiamente, dejando la base de datos lista para ser usada y evitando fugas de memoria o conexiones abiertas.

*********************************************************************************************************************************************************************

##BUSCADOR
En la parte del buscador, lo primero que se hace es tokenizar la búsqueda del usuario. esto es, se separa todo el string de búsqueda en palabras individuales. Si el usuario buscó "100 metros", guardaremos "100" y "metros". 

luego de esto, se hace un query a la base de datos creada por mkindex, llamada index.db. Lo que nuestro código busca, son coincidencias (al menos una vez) por cada palabra tokenizada. si se hizo la búsqueda con "100 metros", nuestro código de fondo buscará todos los documentos que tengan una coincidencia para "100" y otra para "metros". SQlite nos devolverá todos los paths de los archivos donde se cumplieron las condiciones, y nuestro frontend podrá presentarlos al usuario como hipervínculos que llevan al archivo. Estos paths se devuelven por medio de un vector, para contenerlos y poder trabajarlos todos juntos. 
Como el archivo está en formato HTML, se puede visualizar por medio del mismo navegador web del que se accede al servidor autohosteado. No solo tenemos nuestro propio google, sino que tenemos también nuestro propio visualizador de HTML y servidor con archivos. Tenemos casi nuestro propio internet! (a muy baja escala, muy limitado, y extremadamente básico, pero nuestro)

Algo que no fue implementado fue algún tipo de ordenamiento de resultados. De momento, el código devuelve todos los hits sin dar ningún orden o comportamiento definido. queda en manos de SQL y cómo decide devolver el vector de resultados. Como mejora a futuro, se podría implementar un algoritmo que dé mayor peso a palabras clave si están en el titulo del documento, o que ordene por repetición de palabras (esto es, si un documento tiene la palabra buscada 20 veces, probablemente sea más relevante que otro documento donde la palabra buscadda aparece una sola vez. por tanto, deberíamos presentarle al usuario primero los documentos con más hits.)
