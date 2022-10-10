# Exam Rank 06

The goal of **mini_serv** is to create a simple server where client can connect and exchange messages.

The ``extract_message`` and ``str_join`` functions are given to you when passing the exam, they are in ``main.c`` in the subject folder.

## Compilation

You can compile both **mini_serv** with:

```bash
gcc -Wall -Wextra -Werror mini_serv.c -o mini_serv
```

## Usage

You first need to start **mini_serv** on a given port: ``./mini_serv 8081`` for example.

You can then connect with the netcat on the same port as the server ``nc 127.0.0.1 8081``.  
You can type your message and press ``Enter`` and it will be sent to the server.

## Resources

* Your own ``webserv``/``ft_irc`` project
