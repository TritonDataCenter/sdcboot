echo generating post script, esc/p and hp pcl graphics...
nasm -DPOSTSCRIPT -o graph-ps.com graphics.asm
nasm -DEPSON -o graphpin.com graphics.asm
nasm -DHPPCL -o graph-hp.com graphics.asm
echo please copy *.com to your bin directory now :-)
