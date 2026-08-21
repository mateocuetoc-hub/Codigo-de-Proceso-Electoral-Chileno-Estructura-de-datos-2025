/* Proyecto electoral compatible con Turbo C y ANSI C90. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CANDIDATOS 20
#define IDX_SIN_VOTO   -1
#define MIN_FIRMAS_APOYO 32767L

/* Opcional: codigos de ronda/estado si los quieres como int */
#define RONDA_PRIMERA   1
#define RONDA_SEGUNDA   2
#define ELEC_ABIERTA    1
#define ELEC_CERRADA    2
#define ELEC_PROCLAMADA 3

/* ====== Forward declarations (porque hay punteros cruzados) ====== */
struct Candidato;
struct DVotante;
struct NodoMesa;
struct Eleccion;
struct Resultado;
struct Servel;
struct Tricel;
struct SistemaElectoral;

/* ====== Persona ====== */
struct Persona {
    char *rut;
    char *nombre;
    char *nacionalidad;
    int  edad;
};

/* ====== Candidato (pool estatico del Servel) ====== */
struct Candidato {
    struct Persona *datos;
    char *partido;
    char *tipo;     /* "Partido" o "Independiente" */
    long firmasApoyo;
    int  esValido;     /* 1 si aprobado por Servel  0 Si no es aprobado*/
    int  id;           /* indice dentro del pool */
};

/* ====== Votante (lista DOBLEMENTE enlazada por mesa) ====== */
struct DVotante {
    struct Persona *datos;
    int  habilitado;       /* 1 puede votar */
    int  haVotado;         /* 1 ya voto */
    int  idxCandVoto;      /* 0..nCands-1 dentro de la ELECCION, o IDX_SIN_VOTO */
    struct DVotante *ant;
    struct DVotante *sig;
};

/* ====== Mesa (nodo del ABB de una eleccion) ====== */
struct NodoMesa {
    int  idMesa;
    char *comuna;
    char *direccion;

    int  votosCandidatos[MAX_CANDIDATOS]; /* usa 0..(nCands-1) de la eleccion */
    int  totalVotosEmitidos;
    int  votosBlancos;
    int  votosNulos;

    struct DVotante *headV;  /* lista doble: cabeza */
    struct DVotante *tailV;  /* lista doble: cola   */

    struct NodoMesa *izq;    /* ABB por idMesa */
    struct NodoMesa *der;
};

/* ====== Eleccion (nodo de LISTA SIMPLE en Servel) ====== */
struct Eleccion {
    int  id;
    int  ronda;    /* RONDA_PRIMERA / RONDA_SEGUNDA */
    int  estado;   /* ELEC_ABIERTA / ELEC_CERRADA / ELEC_PROCLAMADA */
    struct Candidato *cands[MAX_CANDIDATOS]; /* arreglo compacto de punteros */
    int   nCands;                             /* tamano efectivo del arreglo */
    struct NodoMesa *arbolMesas;              /* ABB propio de esta eleccion */
    struct Resultado * candidato_En_Resultado;
    struct Eleccion *sig;                     /* siguiente eleccion (historial) */
};

/* ====== Resultado (nodo de LISTA CIRCULAR en Tricel) ====== */
struct Resultado {
    struct Candidato *ganador;    /* puntero a uno de eleccion->cands[idxGanador] */
    int   totalMesas;
    int   totalVotantesRegistrados;
    int   totalVotantesVotaron;
    int   votosValidos;
    int   votosBlancos;
    int   votosNulos;
    float porcentajeParticipacion;               /* 0..100 */
    float porcentajeCandidato[MAX_CANDIDATOS];   /* solo 0..(nCands-1) */
    int   idxGanador;                            /* indice dentro del arreglo compacto */
    float porcentajeGanador;
    struct Resultado *sig;   /* anillo: lista circular simplemente enlazada */
};

/* ====== Servel: pool de candidatos + LISTA de elecciones ====== */
struct Servel {
    struct Candidato *candidatos[MAX_CANDIDATOS]; /* pool estatico */
    int   totalCandidatos;
    struct Eleccion *elecciones;                 /* cabeza de la lista simple */
    int   totalVotantesRegistrados;              /* opcional global */
};

/* ====== Tricel: LISTA CIRCULAR de resultados ====== */
struct Tricel {
    struct Resultado *headResultados;  /* NULL si vacio; si 1 nodo: head->sig == head */
    int   totalResultados;
};

/* ====== Sistema (punteros a modulos en heap) ====== */
struct SistemaElectoral {
    struct Servel *servel;
    struct Tricel *tricel;
};

/* ==== PROTOTIPOS de funciones usadas antes ==== */
int validarDatosCanditado(struct Candidato *candidato);

/* ================= FUNCIONES GENERALES ================= */

int verificar_Eleccion_contiene_Candidato(struct Eleccion *eleccion, int idCand)
{
    int i;
    if (eleccion != NULL)
    {
        for (i = 0; i < eleccion->nCands; i++)
        {
            if ( eleccion->cands[i] != NULL && eleccion->cands[i]->id == idCand)
                return 1;
        }
        return 0;
    }
    return 0;
}

struct Candidato *BuscarCandidatoPorId(struct Servel *servel, int idCand)
{
    int i;

    if (servel == NULL)
    {
        return NULL;
    }

    for (i = 0; i < servel->totalCandidatos; i = i + 1)
    {
        if (servel->candidatos[i] != NULL &&
            servel->candidatos[i]->id == idCand)
        {
            return servel->candidatos[i];
        }
    }

    return NULL;
}

void limpiarBuffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
    {
        /* limpiar */
    }
}

/* ================= DATOS DE EJEMPLO ================= */

void inicializarSistemaConDatos(struct SistemaElectoral *sistema)
{
    struct Servel *servel;
    struct Tricel *tricel;
    struct Candidato *c1, *c2;
    struct Persona *p1, *p2;
    struct Eleccion *e1;
    struct NodoMesa *m1;
    struct Resultado *r1;
    int i;
    struct DVotante *v1;
    struct DVotante *v2;
    struct Persona  *p1v;
    struct Persona  *p2v;

    if (sistema == NULL) {
        printf("ERROR: sistema NULL en inicializarSistemaConDatos.\n");
        return;
    }

    servel = sistema->servel;
    tricel = sistema->tricel;

    if (servel == NULL || tricel == NULL) {
        printf("ERROR: servel/tricel NULL en inicializarSistemaConDatos.\n");
        return;
    }

    servel->totalCandidatos = 0;
    servel->elecciones = NULL;
    servel->totalVotantesRegistrados = 0;

    /* ---- Candidato 1 ---- */
    c1 = (struct Candidato *) malloc(sizeof(struct Candidato));
    p1 = (struct Persona  *) malloc(sizeof(struct Persona));

    if (c1 == NULL || p1 == NULL) {
        printf("Error de memoria creando candidato 1.\n");
        return;
    }

    c1->datos = p1;

    p1->rut          = (char *) malloc(12 * sizeof(char));
    p1->nombre       = (char *) malloc(50 * sizeof(char));
    p1->nacionalidad = (char *) malloc(30 * sizeof(char));
    c1->partido      = (char *) malloc(40 * sizeof(char));
    c1->tipo         = (char *) malloc(20 * sizeof(char));

    strcpy(p1->rut,          "11111111");
    strcpy(p1->nombre,       "Alice");
    strcpy(p1->nacionalidad, "chilena");
    p1->edad = 45;
    strcpy(c1->partido,      "PartidoX");
    strcpy(c1->tipo,         "partido");
    c1->firmasApoyo = 0;
    c1->id          = 0;
    c1->esValido    = validarDatosCanditado(c1);

    servel->candidatos[servel->totalCandidatos] = c1;
    servel->totalCandidatos++;

    /* ---- Candidato 2 ---- */
    c2 = (struct Candidato *) malloc(sizeof(struct Candidato));
    p2 = (struct Persona  *) malloc(sizeof(struct Persona));

    if (c2 == NULL || p2 == NULL) {
        printf("Error de memoria creando candidato 2.\n");
        return;
    }

    c2->datos = p2;

    p2->rut          = (char *) malloc(12 * sizeof(char));
    p2->nombre       = (char *) malloc(50 * sizeof(char));
    p2->nacionalidad = (char *) malloc(30 * sizeof(char));
    c2->partido      = (char *) malloc(40 * sizeof(char));
    c2->tipo         = (char *) malloc(20 * sizeof(char));

    strcpy(p2->rut,          "22222222");
    strcpy(p2->nombre,       "Bob");
    strcpy(p2->nacionalidad, "chilena");
    p2->edad = 50;
    strcpy(c2->partido,      "PartidoY");
    strcpy(c2->tipo,         "independiente");
    c2->firmasApoyo = 120000L;
    c2->id          = 1;
    c2->esValido    = validarDatosCanditado(c2);

    servel->candidatos[servel->totalCandidatos] = c2;
    servel->totalCandidatos++;

    /* ===================== ELECCION DE EJEMPLO ===================== */

    e1 = (struct Eleccion *) malloc(sizeof(struct Eleccion));
    if (e1 == NULL) {
        printf("Error de memoria creando eleccion.\n");
        return;
    }

    e1->id     = 100;
    e1->ronda  = RONDA_PRIMERA;
    e1->estado = ELEC_ABIERTA;
    e1->nCands = 0;

    for (i = 0; i < MAX_CANDIDATOS; i++) {
        e1->cands[i] = NULL;
    }

    e1->cands[e1->nCands++] = c1;
    e1->cands[e1->nCands++] = c2;

    e1->arbolMesas = NULL;
    e1->candidato_En_Resultado = NULL;
    e1->sig = NULL;

    e1->sig = servel->elecciones;
    servel->elecciones = e1;

    /* ===================== MESA DE EJEMPLO ===================== */

    m1 = (struct NodoMesa *) malloc(sizeof(struct NodoMesa));
    if (m1 == NULL) {
        printf("Error de memoria creando mesa.\n");
        return;
    }

    m1->idMesa = 10;

    m1->comuna    = (char *) malloc(40 * sizeof(char));
    m1->direccion = (char *) malloc(100 * sizeof(char));

    strcpy(m1->comuna,    "SanFelipe");
    strcpy(m1->direccion, "ColegioCentral123");

    for (i = 0; i < MAX_CANDIDATOS; i++) {
        m1->votosCandidatos[i] = 0;
    }
    m1->totalVotosEmitidos = 0;
    m1->votosBlancos       = 0;
    m1->votosNulos         = 0;
    m1->headV              = NULL;
    m1->tailV              = NULL;
    m1->izq                = NULL;
    m1->der                = NULL;

    e1->arbolMesas = m1;

    /* ===================== RESULTADO EJEMPLO ===================== */

    tricel->headResultados = NULL;
    tricel->totalResultados = 0;

    r1 = (struct Resultado *) malloc(sizeof(struct Resultado));
    if (r1 == NULL) {
        printf("Error de memoria creando resultado.\n");
        return;
    }

    r1->totalMesas               = 1;
    r1->totalVotantesRegistrados = 100;
    r1->totalVotantesVotaron     = 80;
    r1->votosValidos             = 75;
    r1->votosBlancos             = 3;
    r1->votosNulos               = 2;
    r1->porcentajeParticipacion  = 80.0f;

    for (i = 0; i < MAX_CANDIDATOS; i++) {
        r1->porcentajeCandidato[i] = 0.0f;
    }

    r1->porcentajeCandidato[0] = 55.0f;
    r1->porcentajeCandidato[1] = 45.0f;
    r1->idxGanador             = 0;
    r1->porcentajeGanador      = 55.0f;
    r1->ganador                = c1;

    r1->sig = r1;
    tricel->headResultados = r1;
    tricel->totalResultados = 1;

    e1->candidato_En_Resultado = r1;

    printf("Sistema inicializado con datos de ejemplo:\n");
    printf(" - %d candidatos en Servel\n", servel->totalCandidatos);
    printf(" - 1 eleccion (ID=%d) con %d candidatos\n", e1->id, e1->nCands);
    printf(" - 1 mesa (ID=%d) en esa eleccion\n", m1->idMesa);
    printf(" - 1 resultado en Tricel (ganador ID=%d)\n", r1->ganador->id);

    /* ===================== VOTANTES EJEMPLO ===================== */

    v1 = NULL;
    v2 = NULL;
    p1v = NULL;
    p2v = NULL;

    /* --- Votante 1 --- */
    v1 = (struct DVotante *) malloc(sizeof(struct DVotante));
    p1v = (struct Persona  *) malloc(sizeof(struct Persona));

    if (v1 == NULL || p1v == NULL) {
        printf("Error de memoria creando votante 1.\n");
    } else {
        v1->datos = p1v;

        p1v->rut          = (char *) malloc(12 * sizeof(char));
        p1v->nombre       = (char *) malloc(50 * sizeof(char));
        p1v->nacionalidad = (char *) malloc(30 * sizeof(char));

        if (p1v->rut != NULL &&
            p1v->nombre != NULL &&
            p1v->nacionalidad != NULL)
        {
            strcpy(p1v->rut,          "33333333");
            strcpy(p1v->nombre,       "Carlos");
            strcpy(p1v->nacionalidad, "chilena");
            p1v->edad = 30;

            v1->habilitado = 1;
            v1->haVotado   = 0;
            v1->idxCandVoto = IDX_SIN_VOTO;
            v1->ant = NULL;
            v1->sig = NULL;

            if (m1->headV == NULL) {
                m1->headV = v1;
                m1->tailV = v1;
            } else {
                v1->ant        = m1->tailV;
                m1->tailV->sig = v1;
                m1->tailV      = v1;
            }

            servel->totalVotantesRegistrados++;
        } else {
            printf("Error de memoria en cadenas del votante 1.\n");
        }
    }

    /* --- Votante 2 --- */
    v2 = (struct DVotante *) malloc(sizeof(struct DVotante));
    p2v = (struct Persona  *) malloc(sizeof(struct Persona));

    if (v2 == NULL || p2v == NULL) {
        printf("Error de memoria creando votante 2.\n");
    } else {
        v2->datos = p2v;

        p2v->rut          = (char *) malloc(12 * sizeof(char));
        p2v->nombre       = (char *) malloc(50 * sizeof(char));
        p2v->nacionalidad = (char *) malloc(30 * sizeof(char));

        if (p2v->rut != NULL &&
            p2v->nombre != NULL &&
            p2v->nacionalidad != NULL)
        {
            strcpy(p2v->rut,          "44444444");
            strcpy(p2v->nombre,       "Daniela");
            strcpy(p2v->nacionalidad, "chilena");
            p2v->edad = 28;

            v2->habilitado = 1;
            v2->haVotado   = 0;
            v2->idxCandVoto = IDX_SIN_VOTO;
            v2->ant = NULL;
            v2->sig = NULL;

            if (m1->headV == NULL) {
                m1->headV = v2;
                m1->tailV = v2;
            } else {
                v2->ant        = m1->tailV;
                m1->tailV->sig = v2;
                m1->tailV      = v2;
            }

            servel->totalVotantesRegistrados++;
        } else {
            printf("Error de memoria en cadenas del votante 2.\n");
        }
    }

    printf("Se agregaron votantes de ejemplo a la mesa %d.\n", m1->idMesa);
}

/* ================= FUNCIONES SERVEL ================= */

int validarDatosCanditado(struct Candidato *candidato)
{
    int contadorValidador;

    contadorValidador = 0;

    if (candidato != NULL && candidato->datos != NULL)
    {
        if (candidato->datos->edad >= 35)
        {
            contadorValidador++;
        }
        if (candidato->datos->nacionalidad != NULL &&
            strcmp(candidato->datos->nacionalidad, "chilena") == 0)
        {
            contadorValidador++;
        }
        if (candidato->tipo != NULL &&
            (strcmp(candidato->tipo, "independiente") == 0 ||
             strcmp(candidato->tipo, "Independiente") == 0))
        {
            if (candidato->firmasApoyo > MIN_FIRMAS_APOYO)
            {
                contadorValidador++;
            }
            if (contadorValidador == 3)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            if (contadorValidador == 2)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }
    }
    return 0;
}

int validarRut(char *rut)
{
    int len;
    int i;
    int factor;
    int suma;
    int resto;
    int dv_num;
    char dv_input;
    char dv_calc;

    if (rut == NULL) {
        return 0;
    }

    len = (int)strlen(rut);

    if (len < 2 || len > 12) {
        return 0;
    }

    dv_input = rut[len - 1];

    if (dv_input >= 'a' && dv_input <= 'z') {
        dv_input = (char)(dv_input - ('a' - 'A'));
    }

    factor = 2;
    suma   = 0;

    for (i = len - 2; i >= 0; i--) {
        char c;
        int dig;

        c = rut[i];

        if (c < '0' || c > '9') {
            return 0;
        }

        dig = c - '0';
        suma += dig * factor;

        factor++;
        if (factor > 7) {
            factor = 2;
        }
    }

    resto  = suma % 11;
    dv_num = 11 - resto;

    if (dv_num == 11) {
        dv_calc = '0';
    } else if (dv_num == 10) {
        dv_calc = 'K';
    } else {
        dv_calc = (char)('0' + dv_num);
    }

    if (dv_input == dv_calc) {
        return 1;
    }

    return 0;
}

int Servel_inicializarCandidato(struct Servel *servel,
                                struct Candidato **Candidato)
{
    struct Candidato *candidato;
    struct Persona   *persona;

    if (servel == NULL || Candidato == NULL) {
        printf("ERROR: puntero NULL en Servel_inicializarCandidato.\n");
        return 0;
    }

    if (servel->totalCandidatos >= MAX_CANDIDATOS) {
        printf("No hay mas espacio para candidatos.\n");
        return 0;
    }

    candidato = (struct Candidato*) malloc(sizeof(struct Candidato));
    if (candidato == NULL) {
        printf("Error de memoria para candidato.\n");
        return 0;
    }

    persona = (struct Persona*) malloc(sizeof(struct Persona));
    if (persona == NULL) {
        printf("Error de memoria para persona.\n");
        return 0;
    }

    candidato->datos = persona;

    persona->rut          = (char*) malloc(12 * sizeof(char));
    persona->nombre       = (char*) malloc(50 * sizeof(char));
    persona->nacionalidad = (char*) malloc(30 * sizeof(char));
    candidato->partido    = (char*) malloc(40 * sizeof(char));
    candidato->tipo       = (char*) malloc(20 * sizeof(char));

    if (persona->rut == NULL ||
        persona->nombre == NULL ||
        persona->nacionalidad == NULL ||
        candidato->partido == NULL ||
        candidato->tipo == NULL)
    {
        printf("Error de memoria para cadenas del candidato.\n");
        return 0;
    }

    (*Candidato) = candidato;
    return 1;
}

int Servel_cargarDatosCandidato(struct Candidato *candidato)
{
    struct Persona *persona;

    if (candidato == NULL || candidato->datos == NULL) {
        printf("ERROR: candidato o persona NULL en Servel_cargarDatosCandidato.\n");
        return 0;
    }

    persona = candidato->datos;

    printf("Rut (sin puntos, sin guion), no importa si el rut termina con k: ");
    if (scanf("%11s", persona->rut) != 1)
    {
        printf("Rut invalido.\n");
        return 0;
    }
    if (validarRut(persona->rut) == 0)
    {
        printf("Rut invalido.\n");
        return 0;
    }

    printf("Nombre: ");
    if (scanf("%49s", persona->nombre) != 1)
    {
        printf("Nombre invalido.\n");
        return 0;
    }

    printf("Nacionalidad: ");
    if (scanf("%29s", persona->nacionalidad) != 1)
    {
        printf("Nacionalidad invalida.\n");
        return 0;
    }

    printf("Edad: ");
    if (scanf("%d", &persona->edad) != 1)
    {
        printf("Edad invalida.\n");
        return 0;
    }

    printf("Partido: ");
    if (scanf("%39s", candidato->partido) != 1)
    {
        printf("Partido invalido.\n");
        return 0;
    }

    printf("Tipo (Partido/Independiente): ");
    if (scanf("%19s", candidato->tipo) != 1)
    {
        printf("Tipo invalido.\n");
        return 0;
    }

    if (candidato->tipo != NULL &&
        (strcmp(candidato->tipo, "independiente") == 0 ||
         strcmp(candidato->tipo, "Independiente") == 0))
    {
        printf("Cantidad de firmas de apoyo: ");
        if (scanf("%ld", &candidato->firmasApoyo) != 1) {
            printf("Valor invalido, se consideran 0 firmas.\n");
            candidato->firmasApoyo = 0;
        }
    }
    else
    {
        candidato->firmasApoyo = 0;
    }

    return 1;
}

char *txtRonda(int ronda)
{
    switch (ronda) {
        case RONDA_PRIMERA:  return "PRIMERA";
        case RONDA_SEGUNDA:  return "SEGUNDA";
        default:             return "DESCONOCIDA";
    }
}

char *txtEstado(int estado)
{
    switch (estado) {
        case ELEC_ABIERTA:    return "ABIERTA";
        case ELEC_CERRADA:    return "CERRADA";
        case ELEC_PROCLAMADA: return "PROCLAMADA";
        default:              return "DESCONOCIDO";
    }
}

struct Eleccion *buscarEleccionPorId(struct Servel * servel, int id)
{
    struct Eleccion *eleccionact;

    if (servel == NULL) {
        return NULL;
    }

    if (servel->elecciones != NULL)
    {
        eleccionact = servel->elecciones;
        while (eleccionact != NULL)
        {
            if (eleccionact->id == id)
            {
                return eleccionact;
            }
            eleccionact = eleccionact->sig;
        }
        return NULL;
    }
    return NULL;
}

void CambiarEstadoDeEleccion(struct Servel * servel)
{
    int idElec;
    int newEstado;
    struct Eleccion * eleccion;
    int verificador;

    verificador = 1;

    if (servel == NULL)
    {
        printf("Servel NULL\n");
        verificador = 0;
    }

    if (verificador == 1)
    {
        printf("Id de la eleccion a modificar: ");
        if (scanf("%d", &idElec) != 1)
        {
            printf("Id invalido.\n");
            verificador = 0;
        }
    }
    if (verificador == 1)
    {
        eleccion = buscarEleccionPorId(servel, idElec);
        if (eleccion == NULL)
        {
            printf("ELECCION NULL\n");
            verificador = 0;
        }

        if (verificador == 1)
        {
            printf("El estado actual de la Eleccion %d: %s\n",
                   idElec, txtEstado(eleccion->estado));

            printf("Nuevo estado a elegir ( 1 = ABIERTA , 2 = CERRADA , 3 = PROCLAMADA):  ");
            if (scanf("%d", &newEstado) != 1)
            {
                printf("Entrada invalida.\n");
                verificador = 0;
            }
            else if (newEstado != ELEC_ABIERTA &&
                     newEstado != ELEC_CERRADA &&
                     newEstado != ELEC_PROCLAMADA)
            {
                printf("Estado de eleccion invalido ( usa 1, 2 ,3)\n");
                verificador = 0;
            }

            if (verificador == 1)
            {
                eleccion->estado = newEstado;
                printf("ELECCION %d ahora esta en estado: %s\n",
                       eleccion->id, txtEstado(eleccion->estado));
            }
        }
    }
}

/* ================= MENU SERVEL ================= */

void menuServel(struct Servel *servel)
{
    int indice;

    indice = -1;

    if (servel == NULL) {
        printf("ERROR: Servel NULL en menuServel.\n");
        return;
    }

    do
    {
        printf("\n-- SERVEL --\n");
        printf("1) Agregar candidato + validar candidato \n");
        printf("2) Listar candidatos \n");
        printf("3) Crear ELECCION (agregar al historial)\n");
        printf("4) Listar ELECCIONES\n");
        printf("5) Eliminar ELECCION\n");
        printf("6) Cambiar el estado de la Eleccion\n");
        printf("7) Agregar Candidato Valido a una Eleccion\n");
        printf("0) Volver\n");
        printf("Opcion: ");

        if (scanf("%d", &indice) != 1) {
            printf("Entrada invalida.\n");
            limpiarBuffer();
            indice = -1;
            continue;
        }

        switch (indice)
        {
            case 1:
            {
                struct Candidato *candidato;
                int ok;

                candidato = NULL;

                ok = Servel_inicializarCandidato(servel, &candidato);
                if (ok == 0) {
                    break;
                }

                ok = Servel_cargarDatosCandidato(candidato);
                if (ok == 0) {
                    printf("No se pudo completar el registro del candidato.\n");
                    break;
                }

                candidato->id = servel->totalCandidatos;
                servel->candidatos[servel->totalCandidatos] = candidato;
                servel->totalCandidatos++;

                candidato->esValido = validarDatosCanditado(candidato);

                if (candidato->esValido == 1)
                {
                    printf("Candidato agregado con id %d y MARCADO como VALIDO.\n",
                           candidato->id);
                }
                else
                {
                    printf("Candidato agregado con id %d, pero NO cumple los requisitos (esValido=0).\n",
                           candidato->id);
                }

                break;
            }

            case 2:
            {
                int i;
                struct Candidato *candidato;

                if (servel->totalCandidatos == 0) {
                    printf("No hay candidatos en el pool.\n");
                    break;
                }
                for (i = 0; i < servel->totalCandidatos; ++i) {
                    candidato = servel->candidatos[i];
                    if (candidato == NULL )
                    {
                        printf("Error en la lista de candidatos.\n");
                    }
                    else
                    {
                        printf("[%d] ID=%d | RUT=%s | Nombre=%s | Nac=%s | Edad=%d | Tipo=%s | Partido=%s | Firmas=%ld | Valido=%d\n",
                               i,
                               candidato->id,
                               candidato->datos->rut,
                               candidato->datos->nombre,
                               candidato->datos->nacionalidad,
                               candidato->datos->edad,
                               candidato->tipo,
                               candidato->partido,
                               candidato->firmasApoyo,
                               candidato->esValido);
                    }
                }
                break;
            }

            case 3:
            {
                int idElec;
                int ronda;
                int i;
                struct Eleccion *aux;
                struct Eleccion *nueva;

                printf("ID eleccion: ");
                if (scanf("%d", &idElec) != 1) {
                    printf("ID invalido.\n");
                    break;
                }

                printf("Ronda (1=primera, 2=segunda): ");
                if (scanf("%d", &ronda) != 1 || (ronda != 1 && ronda != 2)) {
                    printf("Ronda invalida (usa 1 o 2).\n");
                    break;
                }

                aux = servel->elecciones;
                while (aux != NULL) {
                    if (aux->id == idElec) {
                        printf("Ya existe una eleccion con ese id %d.\n", idElec);
                        break;
                    }
                    aux = aux->sig;
                }
                if (aux != NULL) break;

                nueva = (struct Eleccion*) malloc(sizeof(struct Eleccion));
                if (nueva == NULL)
                {
                    printf("Error de memoria para eleccion.\n");
                    break;
                }

                nueva->id      = idElec;
                nueva->ronda   = ronda;
                nueva->estado  = ELEC_ABIERTA;
                nueva->nCands  = 0;
                for (i = 0; i < MAX_CANDIDATOS; ++i)
                    nueva->cands[i] = NULL;

                nueva->arbolMesas = NULL;
                nueva->candidato_En_Resultado = NULL;
                nueva->sig = servel->elecciones;
                servel->elecciones = nueva;

                printf("Eleccion creada: id=%d, ronda=%d, estado=ABIERTO, nCands=%d\n",
                       nueva->id, nueva->ronda, nueva->nCands);
                break;
            }

            case 4:
            {
                struct Eleccion *eleccion;
                int idx;

                idx = 0;

                if (servel->elecciones == NULL) {
                    printf("No hay elecciones en el historial.\n");
                    break;
                }

                eleccion = servel->elecciones;
                while (eleccion != NULL)
                {
                    printf("#%d -> ID=%d | Ronda=%s | Estado=%s | nCands=%d \n",
                           idx,
                           eleccion->id,
                           txtRonda(eleccion->ronda),
                           txtEstado(eleccion->estado),
                           eleccion->nCands);

                    eleccion = eleccion->sig;
                    idx++;
                }
                break;
            }

            case 5:
            {
                int id;
                struct Eleccion *eleccionAnterior;
                struct Eleccion *eleccionActual;

                eleccionAnterior = NULL;
                eleccionActual  = servel->elecciones;

                printf("ID de la ELECCION a eliminar: ");
                if (scanf("%d", &id) != 1) {
                    printf("ID invalido.\n");
                    break;
                }

                while (eleccionActual != NULL)
                {
                    if (eleccionActual->id == id)
                        break;
                    eleccionAnterior = eleccionActual;
                    eleccionActual  = eleccionActual->sig;
                }

                if (eleccionActual == NULL) {
                    printf("No existe eleccion con id %d.\n", id);
                    break;
                }

                if (eleccionAnterior == NULL) {
                    servel->elecciones = eleccionActual->sig;
                }
                else
                {
                    eleccionAnterior->sig = eleccionActual->sig;
                }
                printf("Eleccion %d eliminada del historial \n", id);
                break;
            }

            case 6:
            {
                CambiarEstadoDeEleccion(servel);
                break;
            }

            case 7:
            {
                int idElec;
                int idCand;
                struct Eleccion  *eleccion;
                struct Candidato *candidato;

                printf("ID de la ELECCION: ");
                if (scanf("%d", &idElec) != 1)
                {
                    printf("ID invalido.\n");
                    limpiarBuffer();
                    break;
                }

                eleccion = buscarEleccionPorId(servel, idElec);
                if (eleccion == NULL)
                {
                    printf("No existe ELECCION con id %d.\n", idElec);
                    break;
                }

                if (eleccion->estado != ELEC_ABIERTA)
                {
                    printf("La ELECCION %d no esta ABIERTA (no se pueden agregar candidatos).\n",
                           idElec);
                    break;
                }

                printf("ID de CANDIDATO a agregar: ");
                if (scanf("%d", &idCand) != 1)
                {
                    printf("ID invalido.\n");
                    limpiarBuffer();
                    break;
                }

                candidato = BuscarCandidatoPorId(servel, idCand);
                if (candidato == NULL)
                {
                    printf("No existe candidato con id %d.\n", idCand);
                    break;
                }

                if (candidato->esValido == 0)
                {
                    printf("El candidato %d NO es valido. Debes validarlo al crearlo.\n",
                           idCand);
                    break;
                }

                if (eleccion->nCands >= MAX_CANDIDATOS)
                {
                    printf("La ELECCION %d ya no admite mas candidatos (MAX_CANDIDATOS).\n",
                           idElec);
                    break;
                }

                if (verificar_Eleccion_contiene_Candidato(eleccion, idCand) == 1)
                {
                    printf("El candidato %d ya esta en la ELECCION %d.\n", idCand, idElec);
                    break;
                }

                eleccion->cands[eleccion->nCands] = candidato;
                eleccion->nCands = eleccion->nCands + 1;

                printf("Candidato id=%d agregado a ELECCION id=%d.\n",
                       candidato->id, eleccion->id);

                break;
            }

            case 0:
                printf("Volviendo al menu principal...\n");
                break;

            default:
                printf("Opcion invalida en Servel.\n");
                break;
        }
    } while (indice != 0);
}

/* ============== FUNCIONES TRICEL / RESULTADOS ============== */

void ContarMesas(struct NodoMesa *mesas, int *contador){
    if (mesas != NULL){
        ContarMesas(mesas->izq, contador);
        (*contador)++;
        ContarMesas(mesas->der, contador);
    }
}

void ContarVotosEmitidos(struct NodoMesa *mesas, int *contadorVotos){
    if (mesas != NULL){
        ContarVotosEmitidos(mesas->izq, contadorVotos);
        (*contadorVotos)+= mesas->totalVotosEmitidos;
        ContarVotosEmitidos(mesas->der, contadorVotos);
    }
}

void ContarVotosNulos(struct NodoMesa *mesas, int *contadorNulos){
    if (mesas != NULL){
        ContarVotosNulos(mesas->izq, contadorNulos);
        (*contadorNulos)+= mesas->votosNulos;
        ContarVotosNulos(mesas->der, contadorNulos);
    }
}

void ContarVotosBlancos(struct NodoMesa *mesas, int *contadorBlancos){
    if (mesas != NULL){
        ContarVotosBlancos(mesas->izq, contadorBlancos);
        (*contadorBlancos)+= mesas->votosBlancos;
        ContarVotosBlancos(mesas->der, contadorBlancos);
    }
}

void ContarXcandidato(struct NodoMesa *mesas, int idCandidato, int *cantidadXcandidato){
    if (mesas != NULL){
        ContarXcandidato(mesas->izq, idCandidato, cantidadXcandidato);
        (*cantidadXcandidato) += mesas->votosCandidatos[idCandidato];
        ContarXcandidato(mesas->der, idCandidato, cantidadXcandidato);
    }
}

int ValidarSegundaVuelta(struct Resultado *resultados, struct Eleccion *eleccionActual)
{
    int i;
    for (i = 0; i < eleccionActual->nCands; i++){
        if(resultados->porcentajeCandidato[i] > 50.00f){
            resultados->ganador = eleccionActual->cands[i];
            resultados->idxGanador = i;
            resultados->porcentajeGanador=resultados->porcentajeCandidato[i];
            return 0;
        }
    }

    return 1;
}

void mostrarResultados(struct Resultado *resultados, struct Eleccion *eleccion)
{
    if (eleccion == NULL) {
        printf("ERROR: No se encontro la eleccion asociada a este resultado.\n");
        return;
    }

    printf("ID de la eleccion = %d\n", eleccion->id);
    printf("Ronda = %s\n",
           eleccion->ronda == RONDA_PRIMERA ? "Primera vuelta" : "Segunda vuelta");

    printf("total de mesas = %d\n", resultados->totalMesas);
    printf("total de votantes registrados = %d\n", resultados->totalVotantesRegistrados);
    printf("total de votos = %d\n", resultados->totalVotantesVotaron);
    printf("total de votos nulos = %d\n", resultados->votosNulos);
    printf("total de votos blancos = %d\n", resultados->votosBlancos);
    printf("total de votos efectivos = %d\n\n", resultados->votosValidos);

    if (resultados->idxGanador != -1 && resultados->ganador != NULL)
    {
        printf("situacion de elecciones = Un candidato supero el 50%%, elecciones terminadas.\n\n");
        printf("GANADOR ELECCION\n");
        printf("ID DEL GANADOR = %d\n", resultados->ganador->id);
        printf("NOMBRE DEL GANADOR = %s\n", resultados->ganador->datos->nombre);
        printf("PORCENTAJE DEL GANADOR = %.2f%%\n\n", resultados->porcentajeGanador);
    }
    else
    {
        printf("situacion de elecciones = Ningun candidato supero el 50%%.\n");
        printf("Se realizara una segunda vuelta.\n\n");
    }
}

void paraSegundaVuelta(struct Resultado *resultados, struct Eleccion *sistema,
                        int *idX, int *idY)
{
    float por1;
    float por2;
    int i;

    por1 = -1.0f;
    por2 = -1.0f;
    *idX = -1;
    *idY = -1;

    for (i = 0;i < sistema->nCands; i++)
    {
        if(resultados->porcentajeCandidato[i] > por1){
            por2 = por1;
            *idY = *idX;

            por1 = resultados->porcentajeCandidato[i];
            *idX = i;
        }
        else if (resultados->porcentajeCandidato[i] > por2){
            por2 =  resultados->porcentajeCandidato[i];
            *idY = i;
        }
    }
}

struct Resultado* recopilarResultados(struct SistemaElectoral *sistema,
                                      struct Eleccion *eleccionActual)
{
    int mesasTotales;
    int votosEmitidos;
    int TvotosBlancos;
    int TvotosNulos;
    int segunda_vuelta;
    int j;
    int i;
    int votosCandidato[MAX_CANDIDATOS];
    struct Resultado *final;
    float participacion;
    int idX;
    int idY;
    struct Eleccion *segunda;

    mesasTotales    = 0;
    votosEmitidos   = 0;
    TvotosBlancos   = 0;
    TvotosNulos     = 0;
    segunda_vuelta  = 0;
    participacion   = 0.0f;
    idX             = -1;
    idY             = -1;
    segunda         = NULL;

    if (sistema == NULL || sistema->servel == NULL || eleccionActual == NULL)
    {
        printf("No se pueden recopilar resultados: datos incompletos.\n");
        return NULL;
    }

    if (eleccionActual->nCands <= 0)
    {
        printf("La eleccion no tiene candidatos.\n");
        return NULL;
    }

    if (eleccionActual->arbolMesas == NULL)
    {
        printf("La eleccion no tiene mesas registradas.\n");
        return NULL;
    }

    for (j = 0; j < MAX_CANDIDATOS; j++) {
        votosCandidato[j] = 0;
    }

    if (sistema->servel->totalVotantesRegistrados == 0)
    {
        return NULL;
    }

    final = (struct Resultado *) malloc(sizeof(struct Resultado));
    if (final == NULL)
    {
        printf("Error de memoria en recopilarResultados.\n");
        return NULL;
    }

    final->ganador = NULL;
    final->idxGanador = -1;
    final->porcentajeGanador = 0.0f;
    final->sig = NULL;

    for (j = 0; j < MAX_CANDIDATOS; j++){
        final->porcentajeCandidato[j]=0.0f;
    }

    ContarMesas(eleccionActual->arbolMesas, &mesasTotales);
    ContarVotosEmitidos(eleccionActual->arbolMesas, &votosEmitidos);
    ContarVotosNulos(eleccionActual->arbolMesas, &TvotosNulos);
    ContarVotosBlancos(eleccionActual->arbolMesas, &TvotosBlancos);

    eleccionActual->candidato_En_Resultado = final;
    final->totalMesas = mesasTotales;
    final->totalVotantesRegistrados = sistema->servel->totalVotantesRegistrados;
    final->totalVotantesVotaron = votosEmitidos;
    final->votosBlancos = TvotosBlancos;
    final->votosNulos = TvotosNulos;
    final->votosValidos = votosEmitidos - TvotosBlancos - TvotosNulos;

    if (final->totalVotantesRegistrados > 0){
        participacion = ((float)votosEmitidos /
                         (float)sistema->servel->totalVotantesRegistrados) * 100.0f;
        final->porcentajeParticipacion = participacion;
    }
    else{
        final->porcentajeParticipacion = 0.0f;
    }

    for (i = 0; i < eleccionActual->nCands; i++) {

        ContarXcandidato(eleccionActual->arbolMesas, i, &votosCandidato[i]);

        if (final->votosValidos > 0) {
            final->porcentajeCandidato[i] =
                ((float)votosCandidato[i] * 100.0f) / (float)final->votosValidos;
        }
        else {
            final->porcentajeCandidato[i] = 0.0f;
        }
    }

    segunda_vuelta = ValidarSegundaVuelta(final,eleccionActual);

    if (eleccionActual->ronda == RONDA_PRIMERA && segunda_vuelta == 1)
    {
        printf("situacion de elecciones = No hay candidatos que superen el 50%% de votos, habra una segunda vuelta.\n\n");

        if (eleccionActual->nCands < 2)
        {
            printf("No se puede crear segunda vuelta: se requieren dos candidatos.\n");
            return final;
        }

        paraSegundaVuelta(final,eleccionActual,&idX, &idY);

        if (idX < 0 || idY < 0)
        {
            printf("No fue posible determinar los candidatos de segunda vuelta.\n");
            return final;
        }

        segunda = (struct Eleccion *) malloc(sizeof(struct Eleccion));
        if (segunda == NULL)
        {
            printf("Error de memoria al crear segunda vuelta.\n");
            return final;
        }

        memset(segunda, 0, sizeof(struct Eleccion));

        segunda->candidato_En_Resultado = NULL;
        segunda->sig = NULL;
        segunda->nCands = 2;

        segunda->cands[0]=eleccionActual->cands[idX];
        segunda->cands[1]=eleccionActual->cands[idY];

        segunda->arbolMesas = eleccionActual->arbolMesas;
        segunda->id = eleccionActual->id + 1000;
        segunda->ronda = RONDA_SEGUNDA;
        segunda->estado = ELEC_ABIERTA;

        segunda->sig = sistema->servel->elecciones;
        sistema->servel->elecciones = segunda;

        printf("SEGUNDA VUELTA CREADA (ID = %d)\n", segunda->id);
        printf("Candidatos que pasan: %s y %s\n\n",
               segunda->cands[0]->datos->nombre,
               segunda->cands[1]->datos->nombre);

        return final;
    }

    printf("HAY UN GANADOR, ELECCIONES TERMINADAS\n\n");
    return final;
}

void  agregarAtricel(struct Tricel * tricel, struct Resultado *resultadoNuevo)
{
    struct Resultado *aux;

    if (tricel == NULL || resultadoNuevo == NULL)
    {
        return;
    }

    if (tricel->headResultados == NULL) {
        tricel->headResultados = resultadoNuevo;
        resultadoNuevo->sig = resultadoNuevo;
        tricel->totalResultados = 1;
        return;
    }

    aux = tricel->headResultados;

    do{
        aux = aux->sig;
    }while(aux->sig != tricel->headResultados);

    aux->sig = resultadoNuevo;

    resultadoNuevo->sig = tricel->headResultados;
    tricel->totalResultados = tricel->totalResultados + 1;

}

struct Eleccion * buscarEleccionPorResultado(struct Servel *servel,
                                             struct Resultado *r)
{
    struct Eleccion *aux;

    aux = servel->elecciones;

    while (aux != NULL) {
        if (aux->candidato_En_Resultado == r) {
            return aux;
        }
        aux = aux->sig;
    }

    return NULL;
}

void resultadoEleccionXid (struct Tricel *sistema, struct Servel *servel,
                           int idBuscado)
{
    struct Resultado *cabeza;
    struct Resultado *recorrido;
    struct Eleccion *elec;

    if (sistema->headResultados == NULL){
        printf("no hay resultados registrados en el sistema\n");
        return;
    }

    cabeza = sistema->headResultados;
    recorrido = sistema->headResultados;

    do {
        elec = buscarEleccionPorResultado(servel, recorrido);

        if (elec != NULL && elec->id == idBuscado){
            mostrarResultados(recorrido,elec);
            return;
        }
        recorrido = recorrido->sig;

    }while(recorrido!= cabeza);

    printf("No existen resultados referente a la id recibida (%d)\n",idBuscado);
}

void proclamarUnGanador (struct Tricel *tricel)
{
    struct Resultado *head;
    struct Resultado *rec;

    if (tricel->headResultados == NULL){
        printf("NO HAY RESULTADOS REGISTRADOS\n\n");
        return;
    }

    head = tricel->headResultados;
    rec = head;

    do {
        rec = rec->sig;
    } while (rec->sig != head);

    if (rec->porcentajeGanador < 50.0f){
        printf("NO HUBO GANADOR EN PRIMERA VUELTA.\n");
        printf("SE DEBE REALIZAR UNA SEGUNDA VUELTA.\n\n");
        return;
    }

    printf("GANADOR ELECCION DE LAS ULTIMAS ELECCIONES REALIZADAS\n\n");
    printf("ID DEL GANADOR = %d\n", rec->ganador->id);
    printf("NOMBRE DEL GANADOR = %s\n", rec->ganador->datos->nombre);
    printf("PORCENTAJE DEL GANADOR = %f\n\n", rec->porcentajeGanador);
}

void listarResultado (struct Tricel *tricel, struct Servel *servel)
{
    struct Resultado *rec;
    struct Eleccion *elec;

    if (tricel->headResultados == NULL){
        printf("NO HAY RESULTADOS EN EL REGISTRO\n\n");
        return;
    }

    printf("LISTA DE RESULTADOS REGISTRADOS EN EL TRICEL\n\n");

    rec = tricel->headResultados;

    do{
        elec = buscarEleccionPorResultado(servel, rec);
        mostrarResultados(rec,elec);
        rec = rec->sig;
    }while(rec != tricel->headResultados);

    printf("\n\n");
    printf("resultados listados\n\n");
}

/* ================= MENU TRICEL ================= */

void MenuTricel( struct Tricel *tricel, struct SistemaElectoral *sistema)
{
    int indice;
    int idB;
    struct Eleccion *aux;
    struct Resultado *resultado;

    indice = - 1;
    idB = 0;
    aux = NULL;
    resultado = NULL;

    do
    {
        printf("\n-- TRICEL --\n");
        printf("1) Generar resultados desde Eleccion\n");
        printf("2) Listar Resultados\n" );
        printf("3) Ver resultado por Id de ELECCION\n");
        printf("4) Proclamar Ganador\n");
        printf("0) Volver\n");
        if (scanf("%d", &indice) != 1)
        {
            limpiarBuffer();
            indice = - 1;
            continue;
        }

        switch (indice)
        {
            case 1:
                printf("Ingrese id de la eleccion a buscar\n\n");
                scanf("%d",&idB);

                aux = sistema->servel->elecciones;

                while (aux != NULL && aux->id != idB){
                    aux = aux->sig;
                }

                if (aux == NULL){
                    printf("No existe una eleccion con ese ID.\n");
                    break;
                }

                resultado = recopilarResultados(sistema, aux);
                if (resultado == NULL)
                {
                    printf("No se pudo generar el resultado.\n");
                }
                else
                {
                    agregarAtricel(tricel, resultado);
                    printf("Generar Resultado, listo\n");
                }
                break;

            case 2:
                listarResultado(tricel, sistema->servel);
                printf("Listar Resultados, listo\n");
                break;

            case 3:
                printf("Ver resultado por id\n");
                printf("Ingrese el ID de la eleccion que desee buscar\n\n");
                scanf("%d",&idB);
                resultadoEleccionXid(tricel, sistema->servel, idB);
                break;

            case 4:
                proclamarUnGanador(tricel);
                printf("Ganador proclamado\n");
                break;

            case 0:
                printf("BREAK\n");
                break;

            default:
                printf("Opcion invalida en Tricel\n");
                break;
        }

    }while (indice != 0);
}

/* ================= FUNCIONES VOTANTES ================= */

struct NodoMesa * buscarLaMesaConId(struct NodoMesa * raiz , int idBuscado)
{
    if (raiz == NULL)
    {
        return NULL;
    }
    if (idBuscado < raiz->idMesa)
    {
        return buscarLaMesaConId(raiz->izq , idBuscado);
    }
    else if (idBuscado > raiz->idMesa)
    {
        return buscarLaMesaConId(raiz->der , idBuscado);
    }
    else
    {
        return raiz;
    }
}

void RegistrarVotanteEnMesa(struct Servel * servel)
{
    int ideleccion;
    int idmesa;
    struct Eleccion *eleccion;
    struct NodoMesa *mesa;
    struct DVotante *votante;
    struct Persona *persona;
    int validador;

    validador = 1;
    ideleccion = 0;
    idmesa = 0;
    eleccion = NULL;
    mesa = NULL;
    votante = NULL;
    persona = NULL;

    if (servel == NULL)
    {
        printf("ERROR: Servel NULL.\n");
        validador = 0;
    }
    else
    {
        printf("Id de la eleccion: ");
        if (scanf("%d", &ideleccion) != 1)
        {
            printf("ID invalido.\n");
            validador = 0;
        }
        if (validador == 1)
        {
            eleccion = buscarEleccionPorId(servel, ideleccion);
            if (eleccion == NULL)
            {
                printf("No existe eleccion con id %d.\n", ideleccion);
                validador = 0;
            }
        }
        if (validador == 1)
        {
            printf("ID de la Mesa: ");
            if (scanf("%d", &idmesa) != 1)
            {
                printf("ID de mesa invalido.\n");
                validador = 0;
            }
        }
        if (validador == 1)
        {
            mesa = buscarLaMesaConId(eleccion->arbolMesas, idmesa);
            if (mesa == NULL)
            {
                printf("No existe mesa con id %d.\n", idmesa);
                validador = 0;
            }
        }
        if (validador == 1)
        {
            votante = (struct DVotante *)malloc(sizeof(struct DVotante));
            if (votante == NULL)
            {
                printf("Error al crear memoria.\n");
                validador = 0;
            }
        }
        if (validador == 1)
        {
            persona = (struct Persona *)malloc(sizeof(struct Persona));
            if (persona == NULL)
            {
                printf("Error al crear memoria.\n");
                validador = 0;
            }
        }
        if (validador == 1)
        {
            votante->datos = persona;
            persona->rut          = (char *) malloc(12 * sizeof(char));
            persona->nombre       = (char *) malloc(50 * sizeof(char));
            persona->nacionalidad = (char *) malloc(30 * sizeof(char));
            if (persona->rut == NULL ||
                persona->nombre == NULL ||
                persona->nacionalidad == NULL)
            {
                printf("Error al crear memoria.\n");
                validador = 0;
            }
        }

        if (validador == 1)
        {
            printf("Rut del Votante , sin puntitos ni guion): ");
            if (scanf("%11s", persona->rut) != 1)
            {
                printf("Rut inv\n");
                validador = 0;
            }
            if (validador == 1 && validarRut(persona->rut) == 0)
            {
                printf("Rut inv\n");
                validador = 0;
            }
        }
        if (validador == 1)
        {
            printf("Nombre del votante: ");
            if (scanf("%49s", persona->nombre) != 1)
            {
                printf("Nombre inv\n");
                validador = 0;
            }
        }
        if (validador == 1)
        {
            printf("Nacionalidad del votante: ");
            if (scanf("%29s" , persona->nacionalidad)!= 1)
            {
                printf("Nacionalidad inv\n");
                validador = 0;
            }
        }
        if (validador == 1)
        {
            printf("Edad del votante: ");
            if (scanf("%d", &persona->edad) != 1 || persona->edad < 0)
            {
                printf("Edad invalida.\n");
                validador = 0;
            }
        }
        if (validador == 1)
        {
            if (persona->edad >= 18 &&
                strcmp(persona->nacionalidad, "chilena") == 0)
            {
                votante->habilitado = 1;
            }
            else
            {
                votante->habilitado = 0;
            }
            votante->haVotado = 0;
            votante->idxCandVoto = IDX_SIN_VOTO;
            votante->ant = NULL;
            votante->sig = NULL;
            if (mesa->headV == NULL)
            {
                mesa->headV = votante;
                mesa->tailV = votante;
            }
            else
            {
                votante->ant = mesa->tailV;
                mesa->tailV->sig = votante;
                mesa->tailV = votante;
            }
            servel->totalVotantesRegistrados++;
            printf("Votante Rut = %s , registrado en Eleccion %d , Mesa %d.\n",
                   persona->rut, eleccion->id, mesa->idMesa);
        }
        else
        {
            printf("No se pudo registrar votante por errores.\n");
        }
    }
}

void ListarVotantesDeMesa(struct Servel *servel)
{
    int idEleccion;
    int idMesa;
    struct Eleccion *eleccion;
    struct NodoMesa *mesa;
    struct DVotante * votante;
    int verificador;

    verificador = 1;
    idEleccion = 0;
    idMesa = 0;
    eleccion = NULL;
    mesa = NULL;
    votante = NULL;

    if (servel == NULL)
    {
        printf("ERROR: Servel NULL en menuVotantes.\n");
        verificador = 0;
    }

    if (verificador == 1)
    {
        printf("Id de la eleccion: ");
        if (scanf("%d", &idEleccion) != 1)
        {
            printf("Entrada inv\n");
            verificador = 0;
        }
    }
    if (verificador == 1)
    {
        eleccion = buscarEleccionPorId(servel, idEleccion);
        if (eleccion == NULL)
        {
            printf("No existe esa Eleccion.\n");
            verificador = 0;
        }
    }
    if (verificador == 1)
    {
        printf("Id de la Mesa: ");
        if (scanf("%d", &idMesa) != 1)
        {
            printf("Entrada inv\n");
            verificador = 0;
        }
    }
    if (verificador == 1)
    {
        mesa = buscarLaMesaConId(eleccion->arbolMesas, idMesa);
        if (mesa == NULL)
        {
            printf("No existe esa Mesa.\n");
            verificador = 0;
        }
    }
    if (verificador == 1)
    {
        if (mesa->headV == NULL)
        {
            printf("Esa Mesa no tiene votantes registrados.\n");
            verificador = 0;
        }
    }
    if (verificador == 1)
    {
        printf("\n--- Votantes de Eleccion %d , Mesa %d ---\n",
               eleccion->id, mesa->idMesa);
        votante = mesa->headV;
        while (votante != NULL)
        {
            if (votante->datos != NULL)
            {
                printf("RUT=%s | Nombre=%s | Nac=%s | Edad=%d | habilitado=%d | haVotado=%d | idxCandVoto=%d\n",
                       votante->datos->rut,
                       votante->datos->nombre,
                       votante->datos->nacionalidad,
                       votante->datos->edad,
                       votante->habilitado,
                       votante->haVotado,
                       votante->idxCandVoto);
            }
            votante = votante->sig;
        }
    }
}

/* ================= MENU VOTANTES ================= */

void menuVotante(struct Servel *servel)
{
    int opcion;

    opcion = -1;

    if (servel == NULL) {
        printf("ERROR: Servel NULL en menuVotantes.\n");
        return;
    }

    do {
        printf("\n-- VOTANTES --\n");
        printf("1) Registrar votante en una mesa\n");
        printf("2) Listar votantes de una mesa\n");
        printf("0) Volver\n");
        printf("Opcion: ");

        if (scanf("%d", &opcion) != 1) {
            printf("Entrada invalida.\n");
            limpiarBuffer();
            opcion = -1;
            continue;
        }

        switch (opcion) {
            case 1:
                RegistrarVotanteEnMesa(servel);
                break;

            case 2:
                ListarVotantesDeMesa(servel);
                break;

            case 0:
                printf("Volviendo al menu principal...\n");
                break;

            default:
                printf("Opcion invalida en Votantes.\n");
                break;
        }

    } while (opcion != 0);
}

/* ============== FUNCIONES PARA REPORTES ============== */

void mostrarResultadosParaVotantes(struct Resultado *resultados,
                                   struct Eleccion *eleccion)
{
    printf("ID de la eleccion = %d\n\n", eleccion->id);

    printf("total de mesas = %d\n", resultados->totalMesas);
    printf("total de votantes registrados = %d\n", resultados->totalVotantesRegistrados);
    printf("total de votos = %d\n", resultados->totalVotantesVotaron);
    printf("total de votos nulos = %d\n", resultados->votosNulos);
    printf("total de votos blancos = %d\n", resultados->votosBlancos);
    printf("total de votos efectivos = %d\n\n", resultados->votosValidos);
}

struct Eleccion *buscarUltimaEleccion(struct Servel *servel)
{
    struct Eleccion *aux;

    aux = servel->elecciones;

    if (aux == NULL){
        return NULL;
    }

    while (aux->sig != NULL){
        aux = aux->sig;
    }

    return aux;
}

void ordenarPorPorcentajeMayorAmenorExchange(struct Candidato **candidatos,
                                             float *porcentaje, int n)
{
    int i, j;
    float auxP;
    struct Candidato *auxC;

    for (i = 0; i < n; i++) {
        for (j = i + 1; j < n; j++) {
            if (porcentaje[i] < porcentaje[j]) {
                auxP = porcentaje[i];
                porcentaje[i] = porcentaje[j];
                porcentaje[j] = auxP;

                auxC = candidatos[i];
                candidatos[i] = candidatos[j];
                candidatos[j] = auxC;
            }
        }
    }
}

void ordenarPorPorcentajeMenorAmayorExchange(struct Candidato **candidatos,
                                             float *porcentaje, int n)
{
    int i, j;
    float auxP;
    struct Candidato *auxC;

    for (i = 0; i < n; i++) {
        for (j = i + 1; j < n; j++) {
            if (porcentaje[i] > porcentaje[j]) {

                auxP = porcentaje[i];
                porcentaje[i] = porcentaje[j];
                porcentaje[j] = auxP;

                auxC = candidatos[i];
                candidatos[i] = candidatos[j];
                candidatos[j] = auxC;
            }
        }
    }
}

void mostrarReportePorcentaje(struct Resultado *resultados,
                              struct Eleccion *eleccion)
{
    int n;
    int i;
    struct Candidato *auxC;
    char *nombre;

    if (resultados == NULL || eleccion == NULL)
    {
        printf("No hay datos de resultados o eleccion.\n");
        return;
    }

    n = eleccion->nCands;

    printf("LISTA DE PORCENTAJES DE LA ULTIMA ELECCION\n");
    printf("-------------------------------------------\n\n");

    printf("PORCENTAJE DE PARTICIPACION: %.2f %%\n\n",
           resultados->porcentajeParticipacion);

    for (i = 0; i < n; i++)
    {
        auxC = eleccion->cands[i];

        if (auxC == NULL || auxC->datos == NULL)
        {
            printf("CANDIDATO #%d: <slot vacio>\n\n", i);
        }
        else
        {
            nombre = auxC->datos->nombre;
            if (nombre == NULL)
            {
                nombre = "(sin nombre)";
            }

            printf("ID CANDIDATO: %d\n", auxC->id);
            printf("NOMBRE: %s\n", nombre);
            printf("PORCENTAJE: %.2f %%\n\n",
                   resultados->porcentajeCandidato[i]);
        }
    }
}

/* ============== MENU REPORTES ============== */

void MenuReportes(struct SistemaElectoral *sistema)
{
    int indice;
    struct Eleccion *aux;
    struct Resultado *res;

    indice = -1;
    aux = NULL;
    res = NULL;

    do
    {
        printf("\n-- REPORTES --\n");
        printf("1) Ordenar candidatos por porcentaje : MAYOR A MENOR (Ultima eleccion)\n");
        printf("2) Ordenar candidatos por porcentaje : MENOR A MAYOR (Ultima eleccion)\n");
        printf("3) Mostrar Resultados de tipo de votos (Ultima eleccion)\n");
        printf("0) Volver\n");

        if (scanf("%d", &indice) != 1)
        {
            limpiarBuffer();
            indice = -1;
            continue;
        }

        aux = buscarUltimaEleccion(sistema->servel);

        if (aux == NULL)
        {
            printf("No hay elecciones registradas.\n");
            continue;
        }

        res = aux->candidato_En_Resultado;

        if (res == NULL)
        {
            printf("No hay resultados para la ultima eleccion.\n");
            continue;
        }

        switch (indice)
        {
            case 1:
                ordenarPorPorcentajeMayorAmenorExchange(aux->cands,
                                                        res->porcentajeCandidato,
                                                        aux->nCands);
                printf("\nOrdenado: MAYOR a MENOR (ExchangeSort).\n");
                mostrarReportePorcentaje(res, aux);
                break;

            case 2:
                ordenarPorPorcentajeMenorAmayorExchange(aux->cands,
                                                        res->porcentajeCandidato,
                                                        aux->nCands);
                printf("\nOrdenado: MENOR a MAYOR.\n");
                mostrarReportePorcentaje(res, aux);
                break;

            case 3:
                printf("\n=== RESULTADOS DE VOTOS ===\n\n");
                mostrarResultadosParaVotantes(res, aux);
                break;

            case 0:
                printf("Volviendo...\n");
                break;

            default:
                printf("Opcion invalida.\n");
        }

    } while (indice != 0);
}

/* ================= MAIN ================= */

int main(void)
{
    struct SistemaElectoral *sistema;
    int Eleccion_Usuario;
    int tipoUsuario;

    Eleccion_Usuario = -1;
    tipoUsuario = 0;

    sistema = (struct SistemaElectoral *)malloc(sizeof(struct SistemaElectoral));
    if (sistema == NULL)
    {
        printf("ERROR al crear sistema.\n");
        return 1;
    }

    sistema->servel = (struct Servel *)malloc(sizeof(struct Servel));
    sistema->tricel = (struct Tricel *)malloc(sizeof(struct Tricel));

    if (sistema->tricel == NULL || sistema->servel == NULL)
    {
        printf("ERROR AL CREAR EL TRICEL/SERVEL\n");
        return 1;
    }

    sistema->servel->totalCandidatos = 0;
    sistema->servel->elecciones = NULL;
    sistema->servel->totalVotantesRegistrados = 0;

    sistema->tricel->headResultados = NULL;
    sistema->tricel->totalResultados = 0;

    inicializarSistemaConDatos(sistema);

    do
    {
        printf("\n===  Seleccione el Usuario ===\n");
        printf("1) Admin\n");
        printf("2) Trabajador del servel\n");
        printf("3) Trabajador del Tricel\n");
        printf("Ingrese su opcion:  ");
        if (scanf("%d", &tipoUsuario) != 1)
        {
            limpiarBuffer();
            printf("Entrada invalida.\n");
            tipoUsuario = 0;
            continue;
        }
        if (tipoUsuario < 1 || tipoUsuario > 3)
        {
            printf("Tipo de usuario invalido , es solo 1 , 2 o 3 \n");
            tipoUsuario = 0;
        }

    }while (tipoUsuario == 0);

    do
    {
        if (tipoUsuario == 1)
        {
            printf("\nSistema Principal\n");
            printf("Seleccione\n");
            printf("0) Salir\n");
            printf("1) Servel\n");
            printf("2) Tricel\n");
            printf("3) Votantes\n");
            printf("4) Reportes\n");
            printf("Opcion: ");
        }
        else if (tipoUsuario == 2)
        {
            printf("\n=== Sistema para Trabajador de Servel ===\n");
            printf("0) Salir\n");
            printf("1) Servel\n");
            printf("2) Votantes\n");
            printf("Opcion: ");
        }
        else if (tipoUsuario == 3)
        {
            printf("\n=== Sistema Para Trabajador de Tricel ===\n");
            printf("0) Salir\n");
            printf("1) Tricel\n");
            printf("2) Reportes\n");
            printf("Opcion: ");
        }

        if (scanf("%d", &Eleccion_Usuario) != 1)
        {
            limpiarBuffer();
            printf("Entrada invalida.\n");
            Eleccion_Usuario = -1;
            continue;
        }

        if (tipoUsuario == 1)
        {
            switch (Eleccion_Usuario)
            {
                case 1:
                    menuServel(sistema->servel);
                    break;
                case 2:
                    MenuTricel(sistema->tricel, sistema);
                    break;
                case 3:
                    menuVotante(sistema->servel);
                    break;
                case 4:
                    MenuReportes(sistema);
                    break;
                case 0:
                    printf("Saliendo...\n");
                    break;
                default:
                    printf("Opcion invalida.\n");
                    break;
            }
        }
        else if (tipoUsuario == 2)
        {
            switch (Eleccion_Usuario)
            {
                case 1:
                    menuServel(sistema->servel);
                    break;
                case 2:
                    menuVotante(sistema->servel);
                    break;
                case 0:
                    printf("Saliendo...\n");
                    break;
                default:
                    printf("Opcion invalida.\n");
                    break;
            }
        }
        else if (tipoUsuario == 3)
        {
            switch (Eleccion_Usuario)
            {
                case 1:
                    MenuTricel(sistema->tricel, sistema);
                    break;
                case 2:
                    MenuReportes(sistema);
                    break;
                case 0:
                    printf("Saliendo...\n");
                    break;
                default:
                    printf("Opcion invalida.\n");
                    break;
            }
        }
    } while (Eleccion_Usuario != 0);

    return 0;
}
