# ZPHR-STM32-0003
LINUM-STM32H753BI

2.2. Assinatura e Criptografia do firmware
Por motivos de segurança, foi adicionada assinatura digital e criptografia aos binários gerados. Primeiro, é necessário gerar um par de chaves pública e privada no formato RSA, que será utilizado para assinar e criptografar a imagem.

Example:

pip install imgtool
imgtool keygen -k boot-signature-rsa.pem -t rsa-2048
imgtool keygen -k encryption-key.pem -t rsa-2048
Em seguida, habilite a assinatura e criptografia no MCUboot e configure as chaves que serão utilizadas

# file sysbuild.conf
#SIGNATURE
SB_CONFIG_BOOT_SIGNATURE_TYPE_RSA=y
SB_CONFIG_BOOT_SIGNATURE_KEY_FILE="${APP_DIR}/boot-signature-rsa.pem"

# Encryption
SB_CONFIG_BOOT_ENCRYPTION=y
SB_CONFIG_BOOT_ENCRYPTION_KEY_FILE="${APP_DIR}/encryption-key.pem"