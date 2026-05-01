
struct peticion_RPC{
    char nombre[256];
    char operacion[256];
    char fichero[256];

};

  program LOGIN  {
      version LOGINVER1 {
         int registrar (struct peticion_RPC) = 1;
      } = 1;
   } = 100525454;
