# Kompilator

#### Autor - Jakub Drzewiecki, 268418

## Opis plików

### *code_generator*

Klasa zajmująca się przekształceniem danych wygenerowanych 
przez klasę *compiler* w graf sterowania przepływem, 
a następnie wygenerowaniem kodu z utworzonego grafu.

### *compiler*

Główna klasa, odpowiedzialna za zapisanie w struktury wszystkie dane,
które pozyskane zostały z parsera. Zajmuje się ona deklaracjami zmiennych, 
procedur, zarządzaniem tablicą symboli.

### *symbol*

Plik nagłówkowy zawierający strukturę danych dla pojedynczego symbolu,
który potem przechowywany będzie w tablicy symboli.

### *symbol_table*

Klasa tablicy symboli, zajmuje się zarządzaniem pamięcią programu
zarezerwowaną dla zadeklarowanych zmiennych.

### *data*

Plik zawierający większość struktur danych tworzonych przez *compiler*
na podstawie programu wejściowego.

### *lexer*

Plik programu flex, zajmujący się parsowaniem tokenów z programu wejściowego.

### *parser* 

Plik programu bison, zajmujący się identyfikacją gramatyki
w proramie wejściowym oraz przekazywaniu informacji do kompilatora.

## Kompilacja i uruchamianie

Kompilator `compiler` skompilować można wywołując w terminalu polecenie `make`

Kompilator uruchomić można poleceniem: 
`./kompilator <nazwa pliku wejsciowego> <nazwa pliku wyjsciowego>`