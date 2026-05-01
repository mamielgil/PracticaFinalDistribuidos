import requests

def proxy_espacio_unico(str1):
    r = requests.post(url="http://127.0.0.1:3000/quitar-espacios",
                  json={ "cadena": str1 },
                  headers={ 'Content-type': 'application/json' })
    if r.status_code == 200:
        return r.text # Devolvemos el string con un único espacio entre palabras
    return None
