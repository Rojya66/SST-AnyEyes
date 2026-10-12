"""Update portable CCS metadata after the SST source tree was reorganized."""
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
tree = ET.parse(ROOT / '.project')
tree.getroot().find('name').text = 'SST_AnyEyes'
tree.write(ROOT / '.project', encoding='utf-8', xml_declaration=True)

tree = ET.parse(ROOT / '.cproject')
includes = ['${PROJECT_ROOT}', '${CG_TOOL_ROOT}/include']
includes += ['${PROJECT_ROOT}/' + p.relative_to(ROOT).as_posix()
             for p in sorted((ROOT / 'Source').rglob('*')) if p.is_dir()]
for opt in tree.findall('.//option'):
    if 'compilerID.INCLUDE_PATH' in opt.get('superClass', ''):
        for child in list(opt):
            opt.remove(child)
        for value in includes:
            ET.SubElement(opt, 'listOptionValue', builtIn='false', value=value)
for entry in tree.findall('.//sourceEntries/entry'):
    entry.set('name', 'Source')
    entry.set('excluding', '|'.join([
        '01_Drive/drivers/' + name + '.c' for name in (
            'usbphyGS60', 'pruss', 'mailbox', 'hsi2c', 'cppi41dma',
            'usbphyGS70', 'gpio_v2', 'uart_irda_cir', 'hs_mmcsd')]
        + ['00_Device/system_config/c674x/cache.c']))
# The linker script is outside the restricted Source source entry.
for opt in tree.findall('.//option'):
    if 'linkerID.LIBRARY' in opt.get('superClass', ''):
        if not any(c.get('value') == '${PROJECT_ROOT}/C6748.cmd' for c in opt):
            ET.SubElement(opt, 'listOptionValue', builtIn='false', value='${PROJECT_ROOT}/C6748.cmd')
tree.write(ROOT / '.cproject', encoding='utf-8', xml_declaration=True)
p = ROOT / '.cproject'
p.write_text(p.read_text(encoding='utf-8').replace('<cproject ', '<?fileVersion 4.0.0?>\n<cproject ', 1), encoding='utf-8')

tree = ET.parse(ROOT / '.ccsproject')
for node in tree.getroot():
    if node.tag == 'origin': node.set('value', ROOT.as_posix())
    if node.tag == 'filesToOpen': node.set('value', 'README.md;Source/04_App/00_Config/ChbBuildConfig.h')
tree.write(ROOT / '.ccsproject', encoding='utf-8', xml_declaration=True)
p = ROOT / '.ccsproject'
p.write_text(p.read_text(encoding='utf-8').replace('<projectOptions>', '<?ccsproject version="1.0"?>\n<projectOptions>', 1), encoding='utf-8')
print('CCS metadata: SST_AnyEyes / C6748 / C6000 8.3.12')
