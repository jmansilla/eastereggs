# eastereggs

## Setup

Create virtual env of your choice and install `requirements.txt`. This is how to do it using venv.

```
cd eastereggs
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install -r requirements.txt
```

### Ngrok

Download ngrok from the [official site](https://dashboard.ngrok.com/get-started/setup/linux).
Make sure the directory where you uncompress the executable (`~/.local/bin`) is in PATH

```
wget https://bin.equinox.io/c/bNyj1mQVY4c/ngrok-v3-stable-linux-amd64.tgz
tar -xvzf ngrok-v3-stable-linux-amd64.tgz -C ~/.local/bin
export PATH="$HOME/.local/bin:$PATH"
```

Now create an account on ngrok and get your token. Add it with:
```
ngrok config add-authtoken <YOUR_AUTH_TOKEN>
```

Then deploy your app to your static domain and using the django port selected. It changes with each user.

```
ngrok http --domain=<YOUR_NGROK_SERVER_URL>.ngrok-free.app 2906 --scheme http,https
```

Finally, make sure your domain is included in the list of `CSRF_TRUSTED_ORIGINS` in `settings.py`
in the django server, or you'll get a CSRF verification failed error.


## Launch django server

If it's the first time you run it (and after altering models in the db), run

```
python manage.py makemigrations
python manage.py migrate
```

Start the Django server under django_site. In zx81 we're using port 2906
```
python manage.py runserver 2906
```

To access the admin, if don't have the credentials run
```
python manage.py createsuperuser
```

### Load groups

To add groups to the database, you can use the command

```
python manage.py load_groups <input_filepath>
```

The file is expected to be in csv format with string columns group_number, group_name, and secret_key. The group number is intended to be just a number (may start with 0). The key validates the identity of the group and
should be hardcoded in their repository.


## Run pingpong

Compile the module using:
```
gcc -o pingpong_example example_client.c
```

Then execute with specific env variables
```
PP_URL="http://localhost:2906/challenge/ping_pong" PP_DEBUG=1 ./pingpong
```

### Handlers

There are three possible actions for groups that we are recording:
- Defeat: they solved the puzzle
- Close:
- Tampering: Records if the ping was tampered with wrong key


### Dashboard

The app dashboards tries to show an aggregate of group stats. It needs serious UI improvements and more charts.

To access it, go to

```
http://localhost:2906/dashboards/dashboard/maindashboard/
```

TODOs:
* Add key to URL for authentication
* You have to load the page twice to see the table because of some error: `Pagination may yield inconsistent results with an unordered object_list: <class 'delay.models.SOGroup'> QuerySet.`
* See when groups have cracked the bug
* See pingpongs per group, filter by tampering attempts
* See pingpongs per group and per user, filter by tampering attempts

## For teachers

  - REMEMBER TO SET the environment variable `PP_DISABLE_EASTER_EGG=1` every time you run your student's code to avoid all that's explained below.
  - You can set the environment variable `PP_URL` for changing the url that the pingpongloop will use. Not recommended, but fyi.
  - You can set the environment variable `PP_DEBUG=1` for having detailed debug prints of what's happening inside the ping_pong_loop function.
  - In the codebase provided to students it's included the file obfuscated.c
  - In some line of the codebase provided, there's a call to `ping_pong_loop` function, with a single `char *` argument set to `NULL`
  - Every time that function is executed, several things happen, but the summary is:
    - the name of the repo is extracted some how from the file system
    - a request is made to a server, using the repository name, and the response will include an increasing number of milliseconds
    - the ping_pong_loop will sleep that amout of milliseconds
    - The callenge is NOT to remove this call, but to disable this easter-egg. They will need to provide a password to the ping_pong_loop for doing that. And that password is obtained from the same server that the ping_pong_loop function is interacting with.

