from setuptools import setup, find_packages

setup(
    name="KungFu_Chess",
    version="0.1",
    packages=find_packages(),
    install_requires=[
        'numpy',
        'keyboard',
        'pytest',
        'pytest-cov'
    ],
) 