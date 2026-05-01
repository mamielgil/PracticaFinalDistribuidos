
struct peticion{
    char nombre[256];
    char operacion[256];
    char fichero[256];

};

  program LOGIN  {
      version LOGINVER1 {
         int registrar (struct peticion) = 1;
      } = 1;
   } = 100525454;
