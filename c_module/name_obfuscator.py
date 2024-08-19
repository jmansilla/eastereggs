from datetime import datetime
import sys

country_names = ['Abkhazia', 'Afghanistan', 'AlandIslands', 'Albania', 'Algeria', 'AmericanSamoa',
 'Andorra', 'Angola', 'Anguilla', 'Antarctica', 'AntiguaBarbuda', 'Argentina', 'Armenia', 'Artsakh',
 'Aruba', 'Australia', 'Austria', 'Azerbaijan', 'Bahamas', 'Bahrain', 'Bangladesh', 'Barbados',
 'Belarus', 'Belgium', 'Belize', 'Benin', 'Bermuda', 'Bhutan', 'Bolivia', 'BosniaHerzegovina',
 'Botswana', 'BouvetIsland', 'Brazil', 'BritishIndianOceanTerritory', 'BritishVirginIslands',
 'Brunei', 'Bulgaria', 'BurkinaFaso', 'Burundi', 'Cambodia', 'Cameroon', 'Canada', 'CapeVerde',
 'CaribbeanNetherlands', 'CaymanIslands', 'CentralAfricanRepublic', 'Chad', 'Chile', 'China',
 'ChristmasIsland', 'Cocos', 'Colombia', 'Comoros', 'Congo', 'CookIslands', 'CostaRica', 'Croatia',
 'Cuba', 'Curazao', 'Cyprus', 'Czechia', 'CotedIvoire', 'Denmark', 'Djibouti', 'Dominica',
 'DominicanRepublic', 'Ecuador', 'Egypt', 'ElSalvador', 'EquatorialGuinea', 'Eritrea', 'Estonia',
 'Eswatini', 'Ethiopia', 'FalklandIslands', 'FaroeIslands', 'Fiji', 'Finland', 'France',
 'FrenchGuiana', 'FrenchPolynesia', 'FrenchSouthernTerritories', 'Gabon', 'Gambia', 'Georgia',
 'Germany', 'Ghana', 'Gibraltar', 'Greece', 'Greenland', 'Grenada', 'Guadeloupe', 'Guam',
 'Guatemala', 'Guernsey', 'Guinea', 'GuineaBissau', 'Guyana', 'Haiti', 'HeardMcDonaldIslands',
 'Honduras', 'HongKongSARChina', 'Hungary', 'Iceland', 'India', 'Indonesia', 'Iran', 'Iraq',
 'Ireland', 'IsleofMan', 'Israel', 'Italy', 'Jamaica', 'Japan', 'Jersey', 'Jordan', 'Kazakhstan',
 'Kenya', 'Kiribati', 'Kosovo', 'Kuwait', 'Kyrgyzstan', 'Laos', 'Latvia', 'Lebanon', 'Lesotho',
 'Liberia', 'Libya', 'Liechtenstein', 'Lithuania', 'Luxembourg', 'MacaoSARChina', 'Madagascar',
 'Malawi', 'Malaysia', 'Maldives', 'Mali', 'Malta', 'MarshallIslands', 'Martinique', 'Mauritania',
 'Mauritius', 'Mayotte', 'Mexico', 'Micronesia', 'Moldova', 'Monaco', 'Mongolia', 'Montenegro',
 'Montserrat', 'Morocco', 'Mozambique', 'Myanmar', 'Namibia', 'Nauru', 'Nepal', 'Netherlands',
 'NewCaledonia', 'NewZealand', 'Nicaragua', 'Niger', 'Nigeria', 'Niue', 'NorfolkIsland',
 'NorthKorea', 'NorthMacedonia', 'NorthernCyprus', 'NorthernMarianaIslands', 'Norway', 'Oman',
 'Pakistan', 'Palau', 'PalestinianTerritories', 'Panama', 'PapuaNewGuinea', 'Paraguay', 'Peru',
 'Philippines', 'PitcairnIslands', 'Poland', 'Portugal', 'PuertoRico', 'Qatar', 'Romania', 'Russia',
 'Rwanda', 'Reunion', 'SahrawiArabDemocraticRepublic', 'Samoa', 'SanMarino', 'SaudiArabia',
 'Senegal', 'Serbia', 'Seychelles', 'SierraLeone', 'Singapore', 'SintMaarten', 'Slovakia',
 'Slovenia', 'SolomonIslands', 'Somalia', 'Somaliland', 'SouthAfrica',
 'SouthGeorgiaSouthSandwichIslands', 'SouthKorea', 'SouthOssetia', 'SouthSudan', 'Spain',
 'SriLanka', 'StBarthelemy', 'StHelena', 'StKittsNevis', 'StLucia', 'StMartin', 'StPierreMiquelon',
 'StVincentGrenadines', 'Sudan', 'Suriname', 'SvalbardJanMayen', 'Sweden', 'Switzerland', 'Syria',
 'SaoTomePrincipe', 'Taiwan', 'Tajikistan', 'Tanzania', 'Thailand', 'TimorLeste', 'Togo', 'Tokelau',
 'Tonga', 'Transnistria', 'TrinidadTobago', 'Tunisia', 'Turkey', 'Turkmenistan',
 'TurksCaicosIslands', 'Tuvalu', 'USOutlyingIslands', 'USVirginIslands', 'Uganda', 'Ukraine',
 'UnitedArabEmirates', 'UnitedKingdom', 'UnitedStates', 'Uruguay', 'Uzbekistan', 'Vanuatu',
 'VaticanCity', 'Venezuela', 'Vietnam', 'WallisFutuna', 'WesternSahara', 'Yemen', 'Zambia',
 'Zimbabwe']

class Obfuscator:
    VAR_TYPES_TO_OBFUSCATE = ['int', 'char', 'short', 'long', 'float', 'double', 'void']

    def __init__(self, source_path, dest_path):
        self.source_path = source_path
        self.dest_path = dest_path
        self.source = open(source_path, 'r').readlines()
        self.result = []
        self.name_generator = NameGenerator()
        self.errors = 0

    def perror(self, *args):
        print('ERROR: ', *args, file=sys.stderr)
        self.errors += 1

    def obfuscate(self):
        self.multiline_comment_started = False
        for i, line in enumerate(self.source):
            self.obfuscate_line(i+1, line)

    def obfuscate_line(self, line_number, line):
        tabulation = ' ' * (len(line) - len(line.lstrip()))  # get number of leading spaces
        line = line.strip()
        if self.multiline_comment_started:
            # omit lines inside multiline comment, but check if it ends
            if '*/' not in line:
                # just inside multiline comment. Skip line
                pass
            else:
                if line.endswith('*/'):
                    self.multiline_comment_started = False
                else: # '*/' in line but not at end
                    return self.perror('*/ closing multiline comment shall be at end of line. Not happening on line', line_number)
            return

        if '/*' in line:
            if line.startswith('/*'):
                self.multiline_comment_started = True
                return self.obfuscate_line(line_number, line.split('/*')[1])  # checking if then closes the comment
            else:
                return self.perror('/* opening multiline comment shall be at start of line. Not happening on line', line_number)


        if line.startswith('//'):
            # skip comment
            return
        elif '//' in line:
            idx = line.index('//')
            if not self.is_inside_string_literal(line, '//', idx):
                line = line.split('//')[0]  # remove comment

        self.detect_name_definitions(line, line_number)
        line = self.apply_renaming(line, line_number)
        self.result.append(tabulation + line.rstrip() + '\n')

    def confirm_obfuscation(self, old_names, line, line_number):
        return True
        print('Please confirm if its ok to rename <%s>' % old_names, 'on line', line_number)
        print(line)
        c = input('Y/n:')
        return c.lower() == 'y'

    def is_inside_string_literal(self, line, subs, idx):
        # Assumes that's true that there's an occurrence of subs in line at idx.
        # Determines if it's inside a string literal
        prev = line[:idx]
        post = line[idx + len(subs):]
        # very simplified analysis. Could be improved
        return prev.count('"') % 2 == 1 and post.count('"') % 2 == 1

    def detect_name_definitions(self, line, line_number):
        # check if there is a new variable or a function definition
        # original_line = line[:]
        if line.startswith('#define'):
            _, name, value = line.split(maxsplit=2)
            if self.confirm_obfuscation(name, line, line_number):
                self.name_generator(name)
            return
            # return '#define ' + new_name + ' ' + value
        for var_type in self.VAR_TYPES_TO_OBFUSCATE:
            for const_or_not in ['', 'const ']:
                prefix_type = const_or_not + var_type
                if line.startswith(prefix_type):
                    # it's a variable or function or a constant definition. Let's capture it.
                    function_data = self.is_function_definition(prefix_type, line, line_number)
                    if function_data:
                        # it's a function definition. function_data is a list of strings
                        if not self.confirm_obfuscation(function_data, line, line_number):
                            continue
                        for name in function_data:
                            self.name_generator(name)
                        return
                    var_or_const = self.is_var_const_definition(prefix_type, line, line_number)
                    if var_or_const:
                        # it's a variable or constant definition. Let's capture it.
                        if not self.confirm_obfuscation(var_or_const, line, line_number):
                            continue
                        self.name_generator(var_or_const)
                        return

    def is_function_definition(self, return_type, line, line_number):
        # returns False if it's not a function definition, or a list of strings if it is, where
        # first element is the function name, and the rest are its parameter-names
        line = line.lstrip()
        if not line.startswith(return_type):
            return False
        open_idx = line.find('(')
        close_idx = line.find(')')
        if open_idx == -1 or close_idx == -1:
            return False
        if open_idx > close_idx:
            return False
        if not line.endswith('{'):
            return False
        # ok, it's a function
        fname = line[:open_idx].strip().replace(return_type, '').strip(' *')
        if not fname:
            return False
        raw_args = line[open_idx+1:close_idx].strip().split(',')
        args = []
        for arg in raw_args:
            arg = arg.strip()
            if arg in ['', '...']: continue
            _type, name = arg.rsplit(maxsplit=1)
            name = name.strip(' *')
            if name:
                args.append(name)
        return [fname] + args

    def is_var_const_definition(self, data_type, line, line_number):
        # returns False if it's not a variable or constant definition
        # and the name of the var or const if it is
        line = line.lstrip()
        if not line.startswith(data_type):
            return False
        if '=' in line:
            # we only care about the part before the =
            line = line.split('=')[0]
        if ',' in line:
            # since we just cropped the line after "=", shall not be a comma. Unless it's a multivar definition.
            # Not supported in version 0.1
            self.perror('Multiple variables defined in same line not supported in version 0.1', line_number)
            return False
        line = line[len(data_type):].strip(' *')
        name = ''
        for char in line:
            if char.isalnum() or char == '_':
                name += char
            else:
                break
        if not name:
            return False

        return name

    def apply_renaming(self, line, line_number):
        for old, new in self.name_generator.memory.items():
            old_len = len(old)
            start = 0
            while old in line and start < len(line):
                idx = line.find(old, start)
                if idx == -1: break
                if self.is_inside_string_literal(line, old, idx):
                    start = idx + old_len
                    continue
                # check if it's in the middle of a word
                prev_pos = idx - 1
                next_pos = idx + old_len
                if prev_pos >= 0 and (line[prev_pos].isalnum() or line[prev_pos] == '_'):
                    start = idx + old_len  # skip this occurence
                    continue
                if next_pos < len(line) and (line[next_pos].isalnum() or line[next_pos] == '_'):
                    start = idx + old_len  # skip this occurence
                    continue
                line = line[:idx] + new + line[idx+old_len:]
        return line

    def write(self):
        lines = filter(lambda x: x not in ['\n', '\r\n', ''], self.result)
        with open(self.dest_path, 'w') as f:
            f.writelines(lines)


class NameGenerator:
    MIN_SIZE_TO_OBFUSCATE = 4
    NAMES_TO_EXCLUDE = ['ping_pong_loop', 'password']
    def __init__(self, family='country_names'):
        if family == 'country_names':
            self.name_list_to_use = country_names[:]
        else:
            self.name_list_to_use = family[:]
        self.memory = {}

    def __call__(self, original_name):
        if original_name in self.NAMES_TO_EXCLUDE:
            return original_name
        if original_name in self.memory:
            return self.memory[original_name]
        if len(original_name) < self.MIN_SIZE_TO_OBFUSCATE:
            return original_name
        name = self.name_list_to_use.pop()
        # some cosmetics
        if original_name.startswith('MAX_'):
            name = 'MAX_' + name
        if original_name.startswith('MIN_'):
            name = 'MIN_' + name
        if original_name.upper() == original_name:
            name = name.upper()
        if original_name.lower() == original_name:
            name = name.lower()
        self.memory[original_name] = name
        return name

if __name__ == '__main__':
    usage = 'Usage: python obfuscator.py <c_file.c> <dest_file.c>'
    if len(sys.argv) < 3:
        print(usage)
        sys.exit(1)
    c_file = sys.argv[1]
    dest_file = sys.argv[2]
    if not c_file.endswith('.c') or not dest_file.endswith('.c'):
        print('Only .c files are supported')
        print(usage)
        sys.exit(1)
    if c_file == dest_file:
        print('Source and destination files must be different')
        print(usage)
        sys.exit(1)

    ob = Obfuscator(c_file, dest_file)
    ob.obfuscate()
    ob.write()

