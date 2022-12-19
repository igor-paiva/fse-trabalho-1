# Servidor central

## Como executar?

Será necessário ter instalado o Make e o GCC.

Utilize o comando `make` para compilar o código.

```
make
```

E para executar pode usar o comando `make run` ou executar o binário diretamente, utilizando o make run será necessário alterar no Makefile os argumentos passados para o binário.

### Make run

```
make run
```

### Binário

```
./bin/prog <<IP DO SERVIDOR>> <<PORTA DO SERVIDOR>>

# Exemplo
./bin/prog localhost 10310
```
