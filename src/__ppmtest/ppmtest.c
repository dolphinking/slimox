#include <stdio.h>
#include <stdint.h>
#include "../slimox-buf.h"
#include "../slimox-rc.h"
#include "../slimox-ppm.h"

#if defined(_WIN32) || defined(_WIN64)
#include <fcntl.h>
#endif

int main(int argc, char** argv) {
    ppm_model_t* model = ppm_model_new();
    rc_coder_t coder;
    buf_t* ib = buf_new();
    buf_t* ob = buf_new();
    int ipos;
    int opos;
    int isize = 0;
    int osize = 0;

#if defined(_WIN32) || defined(_WIN64)
    /* we need to set stdin/stdout to binary mode under windows */
    setmode(fileno(stdin), O_BINARY);
    setmode(fileno(stdout), O_BINARY);
#endif

    if(argc == 2 && strcmp(argv[1], "e") == 0) {
        buf_resize(ib, 100000);
        while((ib->m_size = fread(ib->m_data, 1, ib->m_size, stdin)) > 0) {
            rc_enc_init(&coder);
            ob->m_size = 0;

            for(ipos = 0; ipos < ib->m_size; ipos++) {
                ppm_encode(model, &coder, ib->m_data[ipos], buf_append(ob, output));
                model->m_context <<= 8;
                model->m_context |=  ib->m_data[ipos];
            }
            rc_enc_flush(&coder, buf_append(ob, output));
            fwrite(&ob->m_size, sizeof(ob->m_size), 1, stdout);
            fwrite(&ib->m_size, sizeof(ib->m_size), 1, stdout);
            fwrite(ob->m_data, 1, ob->m_size, stdout);
            isize += ib->m_size;
            osize += ob->m_size;
            fprintf(stderr, "%d KB => %d KB\r", isize / 1000, osize / 1000);
        }
        fprintf(stderr, "\n");
        buf_del(ob);
        buf_del(ib);
        ppm_model_del(model);
    }
    if(argc == 2 && strcmp(argv[1], "d") == 0) {
        buf_resize(ob, 100000);
        while(fread(&ob->m_size, 1, sizeof(ob->m_size), stdin) > 0 &&
              fread(&ib->m_size, 1, sizeof(ib->m_size), stdin) > 0) {
            rc_enc_init(&coder);
            buf_resize(ob, ob->m_size);
            buf_resize(ib, ib->m_size);
            fread(ob->m_data, 1, ob->m_size, stdin);
            ipos = 0;
            opos = 0;
            rc_dec_init(&coder, input = ob->m_data[opos++]);

            while(ipos < ib->m_size) {
                ppm_decode(model, &coder, &ib->m_data[ipos], input = ob->m_data[opos++]);
                model->m_context <<= 8;
                model->m_context |=  ib->m_data[ipos++];
            }
            fwrite(ib->m_data, 1, ib->m_size, stdout);
            isize += ib->m_size;
            osize += ob->m_size;
            fprintf(stderr, "%d KB <= %d KB\r", isize / 1000, osize / 1000);
        }
        fprintf(stderr, "\n");
        buf_del(ob);
        buf_del(ib);
        ppm_model_del(model);
    }
    return 0;
}
