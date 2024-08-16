echo "git pull"
git pull

echo "pip install --upgrade pip"
pip install --upgrade pip

echo "pip install -r production-requirements.txt"
pip install -r production-requirements.txt

echo "pip install -r requirements.txt"
pip install -r requirements.txt

if ! test -f secrets.txt; then
  echo "File secrets.txt does not exist. Create it with the following content:"
  echo "DEBUG = False"
  echo "SECRET_KEY = 'put your secret key here'"
  exit
fi

echo "python manage.py migrate"
python manage.py migrate

echo "python manage.py collectstatic --noinput"
python manage.py collectstatic --noinput

echo "python manage.py runserver 0.0.0.0:8000"
python manage.py runserver 0.0.0.0:8000
