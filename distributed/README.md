# Servidor distribuído

## Como executar?

Será necessário ter instalado o Make e o GCC.

Utilize o comando `make` para compilar o código.

```
make
```

E para executar pode usar o comando `make run` ou executar o binário diretamente, utilizando o make run será necessário alterar no Makefile o argumento passado para o binário.

### Make run

```
make run
```

### Binário

```
./bin/prog <<CAMINHO PARA O ARQUIVO DE INICIALIZAÇÃO>>

# Exemplo
./bin/prog app_data/room_1.json
```
