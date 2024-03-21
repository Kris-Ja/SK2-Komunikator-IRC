
# Komunikator IRC

# Autorzy:

- Krzysztof Jaworski
- Anna Roszkiewicz

# Opis protokołu komunikacyjnego:

Wymiana informacji między serwerem a klientem odbywa się poprzez komunikaty o określonym formacie zakończone znakiem NULL, na przykład:

- S12 cześć\0 - przesłane od klienta do serwera oznacza wysłanie wiadomości o treści "cześć" na kanał o numerze 12
- J2 user\0 - wiadomość o dołączeniu na kanał 2. użytkownika "user" wysyłana przez serwer do członków kanału

Wiadomości są szyfrowane za pomocą protokołu SSL/TLS.

# Opis implementacji:

Wielowątkowy serwer został napisany w języku C. Nowy wątek tworzony jest dla każdego klienta. Kod źródłowy serwera znajduje się w pliku server.c.

Klienta na system Windows napisaliśmy w języku C++, a w celu stworzenia interfejsu graficznego aplikacji wykorzystaliśmy bibliotekę Qt. Kod źródłowy klienta jest w folderze sk2-irc w repozytorium. Zawartość plików źródłowych:

- reader.cpp to klasa odpowiedzialna za odbieranie komunikatów od serwera i wysyłanie sygnałów do elementów aplikacji
- mainpage.cpp zarządza główną stroną projektu
- login.cpp odpowiada za ekran logowania
- namechannel.cpp odpowiada za wyskakujące okienko, w którym użytkownik wpisuje nazwę kanału
- main.cpp uruchamia główny wątek aplikacji i okno logowania

# Sposób kompilacji:

Serwer kompilujemy poleceniem:
```
gcc server.c -pthread -lssl -lcrypto -o nazwa-pliku
```

i uruchamiamy:
```
./nazwa-pliku
```

Klienta kompilujemy poleceniem:
```
cmake -S folder-z-kodem-źródłowym -B . -DCMAKE_PREFIX_PATH=ścieżka-do-kompilatora  -G "MinGW Makefiles"
```

przykładowo:  
```
cmake -S ../sk2-irc -B . -DCMAKE_PREFIX_PATH=C:\Qt2\6.6.1\mingw_64  -G "MinGW Makefiles"
```

Następnie należy wykonać polecenie:
```
cmake --build .
```

# Sposób obsługi klienta:

Po uruchomieniu aplikacji w oknie logowania należy wpisać adres IP, numer portu i nazwę użytkownika. Po przekierowaniu do głównego okna aplikacji można wpisywać wiadomości w pole na dole ekranu i tworzyć kanały przyciskiem Create channel. Pojedyncze kliknięcie na nazwę kanału na liście powoduje dołączenie do kanału, a podwójne kliknięcie wyjście.


