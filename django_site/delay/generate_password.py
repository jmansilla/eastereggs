import random

ANIMALS =['Duck', 'Beagle', 'Koala', 'Mammoth', 'Goose', 'Whale', 'Camel', 'Jellyfish', 'ArcticFox', 'Possum',
          'Goat', 'Elk', 'Rat', 'Baboon', 'Cat', 'Bat', 'Donkey', 'Cheetah', 'Iguana', 'Bull', 'Cobra', 'Fox',
          'Hare', 'Lion', 'Flamingo', 'Zebra', 'Boar', 'Otter', 'PolarBear', 'Buffalo', 'Armadillo', 'Tortoise',
          'Alligator', 'Antelope', 'Sloth', 'Kangaroo', 'Meerkat', 'Racoon', 'Rabbit', 'Tiger', 'Swan', 'Chihuahua',
          'Lemur', 'Emu', 'Albatross', 'Jackal', 'Deer', 'Wombat', 'Sheep', 'Chimpanzee', 'Leopard', 'Dodo', 'Gorilla',
          'HammerheadShark', 'Dolphin', 'Mouse', 'KingCobra', 'Owl', 'Lizard', 'Hen', 'MonitorLizard', 'Lynx', 'Cow',
          'Wolf', 'Mole', 'Vulture', 'Monkey', 'Jaguar', 'Eagle', 'Crow', 'Orangutan', 'Panther', 'Bear', 'Chameleon',
          'Llama', 'Badger', 'Crocodile', 'Ostrich', 'Beaver', 'Rhinoceros', 'Hawk', 'Elephant', 'Mule', 'Snake', 'Eel',
          'Pigeon', 'Hamster', 'HermitCrab', 'Turtle', 'Hippopotamus', 'BlueWhale', 'Ibex', 'Giraffe', 'GiantPanda',
          'Bison', 'Dog', 'Chinchillas', 'Horse', 'Porcupine', 'Hedgehog', 'Fish', 'Frog', 'FlyingSquirrel', 'Peacock',
          ]
COLORS = ['Red', 'Blue', 'Green', 'Orange', 'White', 'Black', 'Yellow', 'Purple', 'Silver', 'Brown', 'Gray', 'Pink',
          'Olive', 'Maroon', 'Violet', 'Charcoal', 'Magenta', 'Bronze', 'Gold', 'Mustard', 'NavyBlue', 'Coral',
          'Burgundy', 'Lavender', 'Cyan', 'Indigo', 'Ruby', 'LimeGreen', 'Salmon', 'Azure', 'Beige', 'CopperRose',
          'Turquoise', 'Aqua', 'Mint', 'SkyBlue', 'LemonYellow', 'Grapevine', 'Fuschia', 'Amber', 'SeaGreen',
          'Pearl', 'Ivory', 'Garnet', 'CherryRed', 'Emerald', 'Brunette', 'Sapphire', 'Lilac', 'ArcticBlue',
          'DarkGreen', 'CoffeeBrown', 'JetBlack', ]


def generate_password():
    animal = ANIMALS[random.randint(0, len(ANIMALS) - 1)]
    color = COLORS[random.randint(0, len(COLORS) - 1)]
    return f'{color}{animal}'


def encrypt(text, salt):
    key = 42 + salt
    new_text = [
        chr(ord(c) ^ key) for c in text
    ]
    return ''.join(new_text)