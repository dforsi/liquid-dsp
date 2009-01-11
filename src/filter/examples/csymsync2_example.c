//
//
//

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../src/filter.h"
#include "../src/firdes.h"

#define DEBUG
#define DEBUG_FILENAME "csymsync2_example.m"

int main() {
    // options
    unsigned int m=5;
    float beta=0.3f;
    unsigned int num_filters=64;
    unsigned int num_symbols=256;

    float bt=0.1f;      // loop filter bandwidth
    float dt=0.5f;      // fractional sample offset
    unsigned int ds=1;  // additional symbol delay
    
    // use random data or 101010 phasing pattern
    bool random_data=true;


    unsigned int k=2;
    unsigned int i, n=0;

    // design interpolating filter
    unsigned int h_len = 2*k*m+1;
    float h[h_len];
    design_rrc_filter(k,m,beta,dt,h);
    float complex hc[h_len];
    for (i=0; i<h_len; i++)
        hc[i] = h[i];

    // create interpolator
    cinterp q = cinterp_create(k,hc,h_len);

    // design polyphase filter
    unsigned int H_len = 2*num_filters*k*m + 1;
    float H[H_len];
    design_rrc_filter(k*num_filters,m,beta,0,H);
    float complex Hc[H_len];
    for (i=0; i<H_len; i++)
        Hc[i] = H[i];
    // create symbol synchronizer
    csymsync2 d = csymsync2_create(num_filters, Hc, H_len);
    csymsync2_set_lf_bw(d,bt);

    unsigned int num_samples = k*num_symbols;
    float complex x[num_symbols];
    float complex y[num_samples];
    float complex z[num_samples+10];

#ifdef DEBUG
    FILE* fid = fopen(DEBUG_FILENAME,"w");
    fprintf(fid,"%% %s, auto-generated file\n\n", DEBUG_FILENAME);
    fprintf(fid,"close all;\nclear all;\n\n");

    fprintf(fid,"k=%u;\n",k);
    fprintf(fid,"m=%u;\n",m);
    fprintf(fid,"beta=%12.8f;\n",beta);
    fprintf(fid,"num_filters=%u;\n",num_filters);
    fprintf(fid,"num_symbols=%u;\n",num_symbols);
#endif

    for (i=0; i<num_symbols; i++) {
        if (random_data) {
            // random signal (QPSK)
            x[i]  = rand() % 2 ? 1.0f : -1.0f;
            x[i] += rand() % 2 ? _Complex_I * 1.0f : -_Complex_I*1.0f;
        } else {
            x[i] = (i%2) ? 1.0f + _Complex_I : -1.0f - _Complex_I;  // 101010 phasing pattern
        }
    }

    // run interpolator
    for (i=0; i<num_symbols; i++) {
        cinterp_execute(q, x[i], &y[n]);
        n+=k;
    }

    // run symbol synchronizer
    unsigned int num_symbols_sync;
    csymsync2_execute(d, &y[ds], num_samples-ds, z, &num_symbols_sync);

    printf("h(t) :\n");
    for (i=0; i<h_len; i++) {
        printf("  h(%2u) = %8.4f;\n", i+1, h[i]);
#ifdef DEBUG
        fprintf(fid,"h(%3u) = %12.5f;\n", i+1, h[i]);
#endif
    }

    printf("x(t) :\n");
    for (i=0; i<num_symbols; i++) {
        //printf("  x(%2u) = %8.4f + j*%8.4f;\n", i+1, crealf(x[i]), cimagf(x[i]));
#ifdef DEBUG
        fprintf(fid,"x(%3u) = %12.5f + j*%12.5f;\n", i+1, crealf(x[i]), cimagf(x[i]));
#endif
    }

    printf("y(t) :\n");
    for (i=0; i<num_samples; i++) {
        //printf("  y(%2u) = %8.4f + j*%8.4f;\n", i+1, crealf(y[i]), cimagf(y[i]));
#ifdef DEBUG
        fprintf(fid,"y(%3u) = %12.5f + j*%12.5f;\n", i+1, crealf(y[i]), cimagf(y[i]));
#endif
    }

    printf("z(t) :\n");
    for (i=0; i<num_symbols_sync; i++) {
        printf("  z(%2u) = %8.4f + j*%8.4f;\n", i+1, crealf(z[i]), cimagf(z[i]));
#ifdef DEBUG
        fprintf(fid,"z(%3u) = %12.5f + j*%12.5f;\n", i+1, crealf(z[i]), cimagf(z[i]));
#endif
    }

#ifdef DEBUG
    fprintf(fid,"\n\n");
    fprintf(fid,"zp = filter(h,1,y);\n");
#if 0
    fprintf(fid,"figure;\nhold on;\n");
    fprintf(fid,"plot([0:length(x)-1],          real(x),    'ob');\n");
    fprintf(fid,"plot([0:length(y)-1]    -m,    real(y),    '-','Color',[0.8 0.8 0.8]);\n");
    fprintf(fid,"plot([0:length(zp)-1]   -2*m,  real(zp),   '-b');\n");
    fprintf(fid,"plot([0:length(z)-1]    -2*m+1,real(z),    'xr');\n");
    fprintf(fid,"hold off;\n");
    fprintf(fid,"xlabel('symbol index');\n");
    fprintf(fid,"ylabel('symbol/signal');\n");
    fprintf(fid,"grid on;\n");
    fprintf(fid,"legend('sym in','interp','mf','sym out',0);\n");
#endif

    fprintf(fid,"tsym=2:2:length(z);\n");
    fprintf(fid,"figure;\n");
    fprintf(fid,"hold on;\n");
    fprintf(fid,"plot(z/k,'-b');\n");
    fprintf(fid,"plot(z(tsym)/k,'rx');\n");
    fprintf(fid,"hold off;\n");
    fprintf(fid,"axis square; grid on;\n");
    fprintf(fid,"xlabel('in-phase');\n");
    fprintf(fid,"ylabel('quadrature');\n");
    fprintf(fid,"legend(['z'],['sym'],1);\n");
    fclose(fid);

    printf("results written to %s.\n", DEBUG_FILENAME);
#endif

    // clean it up
    cinterp_destroy(q);
    csymsync2_destroy(d);
    printf("done.\n");
    return 0;
}
