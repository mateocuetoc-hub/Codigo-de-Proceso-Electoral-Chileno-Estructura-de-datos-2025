#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CANDIDATOS 20
#define IDX_SIN_VOTO   -1
#define LARGO 32767

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
    char *rut[20];
    char *nombre[50];
    char *nacionalidad[30];
    int  edad;
};

/* ====== Candidato (pool estatico del Servel) ====== */
struct Candidato {
    struct Persona *datos;
    char *partido[40];
    char *tipo[20];     /* "Partido" o "Independiente" */
    int  firmasApoyo;
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
    char *comuna[40];
    char *direccion[100];

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

    p1->rut[0]          = (char *) malloc(12 * sizeof(char));
    p1->nombre[0]       = (char *) malloc(50 * sizeof(char));
    p1->nacionalidad[0] = (char *) malloc(30 * sizeof(char));
    c1->partido[0]      = (char *) malloc(40 * sizeof(char));
    c1->tipo[0]         = (char *) malloc(20 * sizeof(char));

    strcpy(p1->rut[0],          "11111111");
    strcpy(p1->nombre[0],       "Alice");
    strcpy(p1->nacionalidad[0], "chilena");
    p1->edad = 45;
    strcpy(c1->partido[0],      "PartidoX");
    strcpy(c1->tipo[0],         "partido");
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

    p2->rut[0]          = (char *) malloc(12 * sizeof(char));
    p2->nombre[0]       = (char *) malloc(50 * sizeof(char));
    p2->nacionalidad[0] = (char *) malloc(30 * sizeof(char));
    c2->partido[0]      = (char *) malloc(40 * sizeof(char));
    c2->tipo[0]         = (char *) malloc(20 * sizeof(char));

    strcpy(p2->rut[0],          "22222222");
    strcpy(p2->nombre[0],       "Bob");
    strcpy(p2->nacionalidad[0], "chilena");
    p2->edad = 50;
    strcpy(c2->partido[0],      "PartidoY");
    strcpy(c2->tipo[0],         "independiente");
    c2->firmasApoyo = 120000;
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

    m1->comuna[0]    = (char *) malloc(40 * sizeof(char));
    m1->direccion[0] = (char *) malloc(100 * sizeof(char));

    strcpy(m1->comuna[0],    "SanFelipe");
    strcpy(m1->direccion[0], "ColegioCentral123");

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

        p1v->rut[0]          = (char *) malloc(12 * sizeof(char));
        p1v->nombre[0]       = (char *) malloc(50 * sizeof(char));
        p1v->nacionalidad[0] = (char *) malloc(30 * sizeof(char));

        if (p1v->rut[0] != NULL &&
            p1v->nombre[0] != NULL &&
            p1v->nacionalidad[0] != NULL)
        {
            strcpy(p1v->rut[0],          "33333333");
            strcpy(p1v->nombre[0],       "Carlos");
            strcpy(p1v->nacionalidad[0], "chilena");
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

        p2v->rut[0]          = (char *) malloc(12 * sizeof(char));
        p2v->nombre[0]       = (char *) malloc(50 * sizeof(char));
        p2v->nacionalidad[0] = (char *) malloc(30 * sizeof(char));

        if (p2v->rut[0] != NULL &&
            p2v->nombre[0] != NULL &&
            p2v->nacionalidad[0] != NULL)
        {
            strcpy(p2v->rut[0],          "44444444");
            strcpy(p2v->nombre[0],       "Daniela");
            strcpy(p2v->nacionalidad[0], "chilena");
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
        if (candidato->datos->nacionalidad[0] != NULL &&
            strcmp(candidato->datos->nacionalidad[0], "chilena") == 0)
        {
            contadorValidador++;
        }
        if (candidato->tipo[0] != NULL &&
            strcmp(candidato->tipo[0], "independiente") == 0)
        {
            if (candidato->firmasApoyo > LARGO)
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

    persona->rut[0]          = (char*) malloc(12 * sizeof(char));
    persona->nombre[0]       = (char*) malloc(50 * sizeof(char));
    persona->nacionalidad[0] = (char*) malloc(30 * sizeof(char));
    candidato->partido[0]    = (char*) malloc(40 * sizeof(char));
    candidato->tipo[0]       = (char*) malloc(20 * sizeof(char));

    if (persona->rut[0] == NULL ||
        persona->nombre[0] == NULL ||
        persona->nacionalidad[0] == NULL ||
        candidato->partido[0] == NULL ||
        candidato->tipo[0] == NULL)
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
    if (scanf("%11s", persona->rut[0]) != 1)
    {
        printf("Rut invalido.\n");
        return 0;
    }
    if (validarRut(persona->rut[0]) == 0)
    {
        printf("Rut invalido.\n");
        return 0;
    }

    printf("Nombre: ");
    if (scanf("%49s", persona->nombre[0]) != 1)
    {
        printf("Nombre invalido.\n");
        return 0;
    }

    printf("Nacionalidad: ");
    if (scanf("%29s", persona->nacionalidad[0]) != 1)
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
    if (scanf("%39s", candidato->partido[0]) != 1)
    {
        printf("Partido invalido.\n");
        return 0;
    }

    printf("Tipo (Partido/Independiente): ");
    if (scanf("%19s", candidato->tipo[0]) != 1)
    {
        printf("Tipo invalido.\n");
        return 0;
    }

    if (candidato->tipo[0] != NULL &&
        strcmp(candidato->tipo[0], "independiente") == 0)
    {
        printf("Cantidad de firmas de apoyo: ");
        if (scanf("%d", &candidato->firmasApoyo) != 1) {
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
    int c;

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
                if (!ok) {
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
                        printf("[%d] ID=%d | RUT=%s | Nombre=%s | Nac=%s | Edad=%d | Tipo=%s | Partido=%s | Firmas=%d | Valido=%d\n",
                               i,
                               candidato->id,
                               candidato->datos->rut[0],
                               candidato->datos->nombre[0],
                               candidato->datos->nacionalidad[0],
                               candidato->datos->edad,
                               candidato->tipo[0],
                               candidato->partido[0],
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
    if (mesas){
        ContarMesas(mesas->izq, contador);
        (*contador)++;
        ContarMesas(mesas->der, contador);
    }
}

void ContarVotosEmitidos(struct NodoMesa *mesas, int *contadorVotos){
    if (mesas){
        ContarVotosEmitidos(mesas->izq, contadorVotos);
        (*contadorVotos)+= mesas->totalVotosEmitidos;
        ContarVotosEmitidos(mesas->der, contadorVotos);
    }
}

void ContarVotosNulos(struct NodoMesa *mesas, int *contadorNulos){
    if (mesas){
        ContarVotosNulos(mesas->izq, contadorNulos);
        (*contadorNulos)+= mesas->votosNulos;
        ContarVotosNulos(mesas->der, contadorNulos);
    }
}

void ContarVotosBlancos(struct NodoMesa *mesas, int *contadorBlancos){
    if (mesas){
        ContarVotosBlancos(mesas->izq, contadorBlancos);
        (*contadorBlancos)+= mesas->votosBlancos;
        ContarVotosBlancos(mesas->der, contadorBlancos);
    }
}

void ContarXcandidato(struct NodoMesa *mesas, int idCandidato, int *cantidadXcandidato){
    if (mesas){
        ContarXcandidato(mesas->izq, idCandidato, cantidadXcandidato);
        (*cantidadXcandidato) += mesas->votosCandidatos[idCandidato];
        ContarXcandidato(mesas->der, idCandidato, cantidadXcandidato);
    }
}

/// Funciones las cuales recorren un arbol binario de mesas usando la recursion, todas estas funciones tienen el mismo funcionamiento
/// el cual es siemopre recorrer el subarbol izquierdo, procesar ese nodo y luego se recorrerian los subarboles mas a la derecha
/// este tipo de recorrido se llama in order tiene como estructura izquierda->nodo->derecha

/// en todas estas funciones no se devuelven valores, el resultado se va acumulado en una variable con puntero la cual es recibida por la funcion  







int ValidarSegundaVuelta(struct Resultado *resultados, struct Eleccion *eleccionActual) //valida si hay o no segunda vuelta
{
    int i;
    for (i = 0; i < eleccionActual->nCands; i++){
        if(resultados->porcentajeCandidato[i] > 50.00f){ /// en caso de que haya un ganador se agregaran sus datos al struct del resultado de esa eleccion 
            resultados->ganador = eleccionActual->cands[i]; /// en caso de que haya un ganador se agregaran sus datos al struct del resultado de esa eleccion 
            resultados->idxGanador = i; /// el id del ganador
            resultados->porcentajeGanador=resultados->porcentajeCandidato[i]; /// el porcentaje con el que gano
            return 0; /// retornara 0 ya que hay un ganador
        }
    }

    return 1;  /// en caso de que no hayan candidatos que superen el 50% en votos se devolvera el valor 1 el cual significaria segunda vuelta
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
        printf("NOMBRE DEL GANADOR = %s\n", resultados->ganador->datos->nombre[0]);
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
{  /// en caso de que no hayan candidatos que superen el 50% en votos se devolvera el valor 1 el cual significaria segunda vuelta
    float por1;
    float por2;
    int i;
    /// variables llamadas por1 y por2 los cuales guardan los 2 porcentajes de votos mas alto, se inician en -1.0 para que cualquier porcentaje real sea mayor
    por1 = -1.0f;
    por2 = -1.0f;
    *idX = -1;  
    *idY = -1;

    for (i = 0;i < sistema->nCands; i++)/// se recorre todo el arreglo de los candidatos desde la posicion 0 hasta nCands
    {
        if(resultados->porcentajeCandidato[i] > por1){ /// en caso de que el porcentaje de votos a un candidato sea mayor que 1...
            por2 = por1; /// el segundo lugar para a ser el primer lugar anterior
            *idY = *idX;  /// ademas se actualizara la id del segundo lugar
            /// se hace esto para evitar que un candidato con menor porcentaje que el antiguo primer lugar quede para la 2da vuelta
            por1 = resultados->porcentajeCandidato[i]; /// se actualizara el porcentaje del primer lugar
            *idX = i; /// se actualizara el id del primer lugar
        }
        else if (resultados->porcentajeCandidato[i] > por2){/// si el porcentaje no fue mayor al del primer lugar, ahora se compararia con el del segundo lugar
            por2 =  resultados->porcentajeCandidato[i]; /// se actualizan los porcentajes e ids del nuevo segundo lugar
            *idY = i;
        }
    }
}


/// un punto importante de esta funcion es que si o si se debe usar el else if y no doble if, porque un candidato no puede ser primer y segundo lugar al mismo tiempo

/// en este caso al usar el if y else if, un candidato que tenga un porcentaje mayor a la variable por1 solo entraria en la primera situacion y no entraria en el segundo
/// si no supera el primer if pero si entra en el else if asi que se podria convertir en 2do lugar

/// el riesgo de usar doble if es que si un candidato supera el primer lugar, automaticamente seria mayor que el segundo lugar y de ese modo entraria al segundo if

/// en resumen se usa else if para evitar que se devuelvan datos "duplicados", en este caso que se devuelvan 2 IDs iguales 







/// solo se le pasa con puntero simple porque no se modifican campos dentro de las estructuras en si
struct Resultado* recopilarResultados(struct SistemaElectoral *sistema,
                                      struct Eleccion *eleccionActual)
{/// funcion que recopila los datos de una eleccion
    int mesasTotales;
    int votosEmitidos;
    int TvotosBlancos;
    int TvotosNulos;
    int segunda_vuelta;
    int j;
    int i;
    int votosCandidato[MAX_CANDIDATOS]; /// arreglo para contar votos por candidato, cada posicion representa un candidato y se empezara desde la posicion 0
    struct Resultado *final; /// se crea una estructura del struct "Resultado" la cual se llenara de informacion recopilada a lo largo de la eleccion, esta al terminar sera retornada al tricel
    float participacion;
    int idX;
    int idY;
    struct Eleccion *segunda; /// en caso de que haya segunda vuelta se usara esta estructura

    mesasTotales    = 0;
    votosEmitidos   = 0;
    TvotosBlancos   = 0;
    TvotosNulos     = 0;
    segunda_vuelta  = 0;
    participacion   = 0.0f;
    idX             = -1;
    idY             = -1;
    segunda         = NULL;

    for (j = 0; j < MAX_CANDIDATOS; j++) {
        votosCandidato[j] = 0;
    }

    if (sistema->servel->totalVotantesRegistrados == 0) /// hay que aseguar que hayan votos en esa eleccion
    {
        return NULL;
    }

    final = (struct Resultado *) malloc(sizeof(struct Resultado));
    if (final == NULL)
    {
        printf("Error de memoria en recopilarResultados.\n");
        return NULL;
    }

    final->ganador = NULL; /// aca se marca que aun no hay ganador
    final->idxGanador = -1; /// se inicia en -1 porque hay candidatos que tienen como ID le 0
    final->porcentajeGanador = 0.0f; /// no se obtiene todavia el porcentaje mayor
    final->sig = NULL;/// al usar lista circular no se apuntara a nadie por ser la ultima eleccion realizada
    ///se inicializan los campos para un inicio seguro y valido 

    for (j = 0; j < MAX_CANDIDATOS; j++){
        final->porcentajeCandidato[j]=0.0f; /// se inician los porcentajes de los candidatos en 0% para evitar datos basura 
    }

    ContarMesas(eleccionActual->arbolMesas, &mesasTotales);
    ContarVotosEmitidos(eleccionActual->arbolMesas, &votosEmitidos);
    ContarVotosNulos(eleccionActual->arbolMesas, &TvotosNulos);
    ContarVotosBlancos(eleccionActual->arbolMesas, &TvotosBlancos);
        /// se cuentan los diferentes datos 


    eleccionActual->candidato_En_Resultado = final;
    final->totalMesas = mesasTotales;
    final->totalVotantesRegistrados = sistema->servel->totalVotantesRegistrados;
    final->totalVotantesVotaron = votosEmitidos;
    final->votosBlancos = TvotosBlancos;
    final->votosNulos = TvotosNulos;
    final->votosValidos = votosEmitidos - TvotosBlancos - TvotosNulos;
    /// se completan los campos con los datos obtenidos

    /// se calcula el porcentaje de participacion de las elecciones actuales
    if (final->totalVotantesRegistrados > 0){
        participacion = ((float)votosEmitidos /
                         (float)sistema->servel->totalVotantesRegistrados) * 100.0f;
        final->porcentajeParticipacion = participacion;
    }
    else{
        final->porcentajeParticipacion = 0.0f;
    }
    /// es importante validar que el total de votos sea mayor a 0 ya que no se puede hacer divison de algo por 0

    for (i = 0; i < eleccionActual->nCands; i++) {  ///  aca se cuentan y de paso de calculan los porcentajes de votos de cada candidato de la eleccion

        ContarXcandidato(eleccionActual->arbolMesas, i, &votosCandidato[i]);

        if (final->votosValidos > 0) {
            final->porcentajeCandidato[i] =
                ((float)votosCandidato[i] * 100.0f) / (float)final->votosValidos;
        }
        else {
            final->porcentajeCandidato[i] = 0.0f;
        }
    }

    segunda_vuelta = ValidarSegundaVuelta(final,eleccionActual); /// SE METEN TODOS LOS DATOS DE LA ELECCION PA CORROBORAR SI HAY SEGUNDA VUELTA O NO

    if (eleccionActual->ronda == RONDA_PRIMERA && segunda_vuelta == 1) /// EN CASO DE QUE HAYA SEGUND VUELTA
    {
        printf("situacion de elecciones = No hay candidatos que superen el 50%% de votos, habra una segunda vuelta.\n\n");

        paraSegundaVuelta(final,eleccionActual,&idX, &idY); /// ACA SE BUSCAN LOS 2 CANDIDATOS CON MAYOR PORCENTAJE DE VOTOS

        segunda = (struct Eleccion *) malloc(sizeof(struct Eleccion)); /// SE CREA UNA ESTRUCTURA PARA LA 2DA VUELTA DE LA ELECCION
        if (segunda == NULL)
        {
            printf("Error de memoria al crear segunda vuelta.\n");
            return final;
        }

        memset(segunda, 0, sizeof(struct Eleccion)); // SE UTILIZA MEMSET PARA INICIALIZAR TODO EN 0 o NULL

        segunda->candidato_En_Resultado = NULL;
        segunda->sig = NULL;
        segunda->nCands = 2;

        segunda->cands[0]=eleccionActual->cands[idX];
        segunda->cands[1]=eleccionActual->cands[idY]; 

        segunda->arbolMesas = eleccionActual->arbolMesas; /// SE UTILIZARAN LAS MISMAS MESAS 
        segunda->id = eleccionActual->id + 1000;
        segunda->ronda = RONDA_SEGUNDA;
        segunda->estado = ELEC_ABIERTA;

        segunda->sig = sistema->servel->elecciones; /// esta linea se traduce a que la siguiente de la nueva eleccion ser la que antes era la primera en la lista
        sistema->servel->elecciones = segunda; ///se mueve la cabeza de la lista para que apunte a la eleccion nueva, osea la segunda vuelta quedaria registrada como la mas reciente

        /// lineas que ayudan a insertar la eleccion de la 2da vuelta en el sistema, si se llegaran a omitir se podrian crear ciclos infinitos, romper la lista o incluso perder los datos de elecciones anteriores
        
        /// en caso de omitir la primera linea se perderian las elecciones antiguas
        
        /// si no se usa la segunda linea se apunta bien al inicio pero la cabecera nunca cambiaria, por esto no se insertaria nada y provocaria una fuga de memoria
        ///en otras palabras esa eleccion quedaria desconectada y no se podria usar nunca




        printf("SEGUNDA VUELTA CREADA (ID = %d)\n", segunda->id);
        printf("Candidatos que pasan: %s y %s\n\n",
               segunda->cands[0]->datos->nombre[0],
               segunda->cands[1]->datos->nombre[0]);

        return final; /// SE RETORNARA EL RESULTADO DE LA ELECCION AUNQUE NO HAYA HABIDO GANADOR
    }

    printf("HAY UN GANADOR, ELECCIONES TERMINADAS\n\n");
    return final; /// EN CASO DE QUE HAYA GANADOR SE RETORNARA EL RESULTADO FINAL CON EL GANADOR YA PROCLAMADO 
}





/// solo se le pasa con puntero simple porque no se modifican campos dentro de las estructuras en si
void  agregarAtricel(struct Tricel * tricel, struct Resultado *resultadoNuevo)
{// agrega los resultados de una eleccion al tricel
    struct Resultado *aux;

    if (tricel->headResultados == NULL) {/// en caso de que no hayan datos en la lista circular de resultados
        tricel->headResultados = resultadoNuevo; /// el nuevo nodo es la nueva cabecera de la lista circular
        resultadoNuevo->sig = resultadoNuevo; /// al ser lista circular se debe apuntar a si mismo, si no se hace esto se perderia el concepto de lista circular
        return;
    }
    /// si no se hace esto, el aux se iniciara en null y nunca empezaria a recorrer la lista, el aux->sig seria igual a SEGFAULT y se caeria el programa
    /// no se insertaria nunca el primer nodo, en el ciclo while siempre se asumira que hay un ultimo nodo pero en verdad no existe ninguno
    aux = tricel->headResultados;

    do{
        aux = aux->sig;
    }while(aux->sig != tricel->headResultados);
    /// se usa do while porque en la lista circular se debe recorrer al menos un nodo antes de evaluar la condicion 
    /// si se usara solo while el ciclo no se ejecutaria en ciertos casos, como ejemplo cuando el aux ya esta ultimo en la lista (siempre hay que entrar al menos una vez a la lista circular)

    aux->sig = resultadoNuevo; ///aca se conecta el ultimo nodo al nuevo
    /// si no se hace esto, el nuevo nodo no entraria nunca en la lista
    resultadoNuevo->sig = tricel->headResultados;/// aca se cierra "el ciclo circular"
    ///si no se hace esto la lista circular se perderia y se convertiria en una lista normal, la cual probablemente este rota y probablemente genere inestabilidad en el programa

}

struct Eleccion * buscarEleccionPorResultado(struct Servel *servel,
                                             struct Resultado *r)
{
    struct Eleccion *aux;

    aux = servel->elecciones;

    while (aux != NULL) { /// no se usa do while en lista simple, ya que al entrar a la lista normal de una podria  y estar vacia, podria hacer que salga el error de SEGFAUL el ejecutar aux == NULL
        if (aux->candidato_En_Resultado == r) {
            return aux;
        }
        aux = aux->sig;
    }

    return NULL; /// si no se encuentra la eleccion se retornara null
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
    }/// se hace esto para evitar el segfault mas adelante

    cabeza = sistema->headResultados;
    recorrido = sistema->headResultados;

    do {/// se usa do while porque en una lista circular nunca habra NULL, si hay un null entremedio significa que la lista esta rota o la estructura dejo de ser circular
        elec = buscarEleccionPorResultado(servel, recorrido);///se busca el resultado de la eleccion

        if (elec != NULL && elec->id == idBuscado){
            mostrarResultados(recorrido,elec);
            return; /// se retornada de la funcion si ya se encontro la eleccion buscada
        }
        recorrido = recorrido->sig;

    }while(recorrido!= cabeza);
     /// se usa do while porque en la lista circular se debe recorrer al menos un nodo antes de evaluar la condicion 
    /// si se usara solo while el ciclo no se ejecutaria en ciertos casos, como ejemplo cuando el aux ya esta ultimo en la lista (siempre hay que entrar al menos una vez a la lista circular)

    printf("No existen resultados referente a la id recibida (%d)\n",idBuscado);
}

void proclamarUnGanador (struct Tricel *tricel) 
{
    if (tricel->headResultados == NULL){
        printf("NO HAY RESULTADOS REGISTRADOS\n\n");
        return;
    }

    struct Resultado *head = tricel->headResultados;
    struct Resultado *rec = head;

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
    printf("NOMBRE DEL GANADOR = %s\n", rec->ganador->datos->nombre[0]);
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
                agregarAtricel(tricel, resultado);

                printf("Generar Resultado, listo\n");
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
            persona->rut[0]          = (char *) malloc(12 * sizeof(char));
            persona->nombre[0]       = (char *) malloc(50 * sizeof(char));
            persona->nacionalidad[0] = (char *) malloc(30 * sizeof(char));
            if (persona->rut[0] == NULL ||
                persona->nombre[0] == NULL ||
                persona->nacionalidad[0] == NULL)
            {
                printf("Error al crear memoria.\n");
                validador = 0;
            }
        }

        if (validador == 1)
        {
            printf("Rut del Votante , sin puntitos ni guion): ");
            if (scanf("%11s", persona->rut[0]) != 1)
            {
                printf("Rut inv\n");
                validador = 0;
            }
            if (validador == 1 && validarRut(persona->rut[0]) == 0)
            {
                printf("Rut inv\n");
                validador = 0;
            }
        }
        if (validador == 1)
        {
            printf("Nombre del votante: ");
            if (scanf("%49s", persona->nombre[0]) != 1)
            {
                printf("Nombre inv\n");
                validador = 0;
            }
        }
        if (validador == 1)
        {
            printf("Nacionalidad del votante: ");
            if (scanf("%29s" , persona->nacionalidad[0])!= 1)
            {
                printf("Nacionalidad inv\n");
                validador = 0;
            }
        }
        if (validador == 1)
        {
            votante->habilitado = 1;
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
                   persona->rut[0], eleccion->id, mesa->idMesa);
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
                       votante->datos->rut[0],
                       votante->datos->nombre[0],
                       votante->datos->nacionalidad[0],
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
    int c;

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
            nombre = auxC->datos->nombre[0];
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
