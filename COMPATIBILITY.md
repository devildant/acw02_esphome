# ACW02 ESPHome Compatibility List

_This file is generated from issues labeled `Compatibility`._

## Airton

| Commercial Model | Ref Model | Status | Notes |
|------------------|-----------|--------|-------|
| 409941 | 3400w (1 unit)  : SMVH12B-3A2A3NG(I)  2500w(3 unit): SMVH09B-2A2A3NG(I) | ✅ Yes | Works great with an Airton quadri-split system.   I used an ADUM1201 for the logic level conversion (3.3V/5V) and a Mini560 step-down converter to drop the 12V line to 5V. I successfully deployed this hardware setup on all 4 indoor units without any issues.  **Specific functions status:** * **Horizontal swing:** Does not work because the indoor units are not physically equipped with it. * **Self-Clean:** Does not work, which is normal as the official manual states these models do not feature it.  <img width="689" height="556" alt="Image" src="https://github.com/user-attachments/assets/9991cc17-4620-4f01-bba6-1cd413372db1" /> |
| 409936 | SMVH18B-4B2A3NM(I) | ✅ Yes | Le module fonctionne correctement, il y a bien toutes les fonctions. Par contre, je n'ai pas de jus venant de la clim, j'ai du alimenter le module via le port USB. Je ne sais pas si c'est lié à ma clim ou à ttes les clim de cette référence. |
| 409683 | SMVH12B-2A2A3NH | ✅ Yes | _No response_ |
| 409944 | N/A | ⚠️ Partially (some functions missing) | This AC use the 409945 module, this module use tuya MCU, look this doc  **[ACW02 ESPHome Tuya MCU integration](./docs/TUYA_MCU_SETUP.md)** |
| 409943 | SMVH12L-3A1A3NB(I) | ⚠️ Partially (some functions missing) | This AC use the 409945 module, this module use tuya MCU, look this doc  **[ACW02 ESPHome Tuya MCU integration](./docs/TUYA_MCU_SETUP.md)** |
| 409942 | N/A | ⚠️ Partially (some functions missing) | This AC use the 409945 module, this module use tuya MCU, look this doc  **[ACW02 ESPHome Tuya MCU integration](./docs/TUYA_MCU_SETUP.md)** |
| 409913 | SMVH09B-2A2A3NG | ✅ Yes | _No response_ |
| 409730 | SMVH09B-2A2A3NH(I) | ✅ Yes | _No response_ |
| 409733 | SMVH12B-3A2A3NG(I) | ✅ Yes | _No response_ |
| 409731 | SMVH12B-2A2A3NH(I) | ✅ Yes | _No response_ |

## Teknopoint

| Commercial Model | Ref Model | Status | Notes |
|------------------|-----------|--------|-------|
| TW-SKIV-07 | N-TW-SMVH07A-2C1A3NG(I) | ✅ Yes | _No response_ |
| TW-SKIV-12 | - | ✅ Yes | No ref model found |
| TW-SKIV-12 | N-TW-SMVH12A-3C1A3NG(I) | ✅ Yes | _No response_ |

